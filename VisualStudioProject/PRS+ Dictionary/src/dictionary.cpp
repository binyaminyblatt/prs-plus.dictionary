/**
 * PRS+ dictionary file format is:
 *  
 *  (all offsets are absolute)
 * 
 *  [header]
 *  [articles]
 *  [word list]
 *  [radix]
 *  
 *  header : 
 *  	"PRSPDICT" (ascii)
 *		header size (uint16)
 *  	version lo (uint8) 
 *  	version hi (uint8)
 *		word list offset (uint32)
 *  	radix offset (uint32)
 *  	... rest is padded with zeros up to 1024 bytes
 *  
 *  articles : article*
 *   
 *  article:
 *  	length (unit32)
 *  	article (utf8 text)
 *  
 *  word list:
 *   	name (UTF8)
 *   	\0
 *   	short translation (up to SHORT_TRANSLATION_LEN chars, UTF8)
 *   	\0
 *  
 *  radix: node*
 *  
 *  node: 
 *  	length - size of the structure in bytes (uint16)
 *  	article offset - (uint32)
 *  	word list offset - (uint32)
 *  	number of child nodes - (uint8)
 *  	offsets of child nodes (uint32 * number of child nodes)
 *  	zero terminated UTF16 names of child nodes (length can be determined by total length of the node)
 *  
 * @author kartu
 */


// For Microsoft compiler not warn about deprecated functions (Microsoft's alternative isn't portable)
#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>

#include <stdlib.h>
#ifndef _MSC_VER
#include <stdint.h>
#else
#include "stdint.h"
#include <windows.h>
#endif
#include "unicode.h"
#include "desaturate.h"

#define ERR_FILE_NOT_FOUND -1
#define ERR_INTERNAL_ERROR -2
#define ERR_INVALID_ARGUMENT -3
#define ERR_UNSUPPORTED_VERSION -4
#define ERR_INVALID_MAGIC -5

#define OFFSET_NCHILDREN 6
#define WORD_LIST_MAX 20
#define WORD_LIST_BUF_LEN WORD_LIST_MAX*256*2
#define MAX_EXACT_MATCH_RECURSIVE_CALLS 1000

#pragma pack(1)

// Dictionary file header
struct Header {
	uint8_t magic[8];
	uint16_t size;
	uint8_t version_lo;
	uint8_t version_hi;
	uint32_t offset_word_list;
	uint32_t offset_radix;
	uint8_t junk[1004];
};

// Radix entry
struct Node {
	uint16_t len; // length of the structure
	uint32_t valueArticle; // pointer to the article (offset in file)
	uint32_t valueWordList; // pointer to the word list entry (offset in file)
	uint8_t nchildren; // number of child nodes
	uint32_t *pchildren; // pointers to child nodes (offset in file)
	int pchildren_len; // size of pchildren
	uint16_t *children_names; // zero terminated UTF-16 names
	int children_names_len; // length of children_names

	Node() {
		pchildren_len = 0;
		children_names_len = 0;
		pchildren = NULL;
		children_names = NULL;
	}
};

// Globals
Header header;
FILE* f;
int n_disk_accesses = 1;
int size_of_node = sizeof(Node) - sizeof(uint32_t*) - sizeof(uint16_t*) - 2 * (sizeof(int));
int size_of_pchildren = sizeof(uint32_t[1024]);

void print_usage() {
	printf("Usage:\n");
	printf("\t<dict.file> e <word> - find exact match, if nothing found, dump list\n");
	printf("\t<dict.file> l <word> - dump list\n");
	printf("\t<dict.file> n <offset> - dump next word list starting from <offset>\n");
	printf("\t<dict.file> p <offset> - dump previous word list, ending at offset <offset>\n");
	printf("\t<dict.file> x <offset> - dump article at address x\n");
}


/**
* Seeks absolute position <pos> in <f> (global FILE variable). 
* Exits program with ERR_INTERNAL_ERROR in case of errors. (broken index, io errors)
*/
void doseek(int pos) {
	if (fseek(f, pos, SEEK_SET)) {
		printf("Error seeking: ferror=%d feof=%d fpos=%d", ferror(f), feof(f), pos);
		exit(ERR_INTERNAL_ERROR);
	}
}

/**
* Reads <size> bytes into <buf> from <f> (global FILE variable). 
* Exits program with ERR_INTERNAL_ERROR in case of errors.
*/
void doread(void* buf, int size) {
	if (size == 0) {
		return;
	}
	n_disk_accesses++;
	int result = (int) fread(buf, size, 1, f);
	if (result != 1) {
		fpos_t fpos;
		fgetpos(f, &fpos);
		printf("Error reading: ferror=%d feof=%d fpos=%d", ferror(f), feof(f), fpos);
		exit(ERR_INTERNAL_ERROR);
	}
}

/**
* Reads <Node> from from <f> (global FILE variable)
*/
void read_node(Node& node) {
	doread(&node, size_of_node);
	int pchildren_len = sizeof(uint32_t*)* node.nchildren;
	int children_names_len = node.len - size_of_node - pchildren_len;

	// ensure pchildren is big enough
	if (node.pchildren_len < pchildren_len) {
		if (node.pchildren_len > 0) {
			free(node.pchildren);
		}
		node.pchildren = (uint32_t*) malloc(pchildren_len);
	}

	// ensure children_names is big enough
	if (node.children_names_len < children_names_len) {
		if (node.children_names_len > 0) {
			free(node.children_names);
		}
		node.children_names = (uint16_t*) malloc(children_names_len);
	}

	doread(node.pchildren, pchildren_len);
	doread(node.children_names, children_names_len);
}

/**
* Finds matching node in the list referenced by <node>. Returns -1 if no matching node were found.
*	node - node to examine
*	search_str - pointer to UTF16 string
*	str_len - length (in words) of UTF16 string
*	match_count - output variable, returns number of matched characters
*	returns index of the matching node, or -1 if none can be matched
*/
int find_matching_child (Node node, uint16_t* search_str, int str_len, int& matched_count, bool normalize = false, int starting_idx = -1) {
	uint16_t* pchildren = &(node.children_names[0]);
	
	// Skip pchildren to starting idx
	for (int i = 0; i < starting_idx + 1; i++) {
		while (*pchildren++ != 0);
	}

	for (int i = starting_idx + 1, n = node.nchildren; i < n; i++) {
		// Compare
		int j;

		bool mismatch = false;
		for (j = 0; j < str_len; j++) {
			// Reached end of string
			if (pchildren[j] == 0) {
				matched_count = j;
				return i;
			}

			// Reached mismatching character
			if (normalize) {
				if ( desaturate(pchildren[j]) != desaturate(search_str[j]) ) {
					mismatch = true;
					break;
				}
			} else {
				if (pchildren[j] != search_str[j]) {
					mismatch = true;
					break;
				}
			}
		}

		// node's string longer than search string?
		if (str_len == j && pchildren[j] != 0) {
			mismatch = true;
		}

		if (mismatch) {
			// Skip to next 0
			while (*pchildren++ != 0);
			continue;
		}

		matched_count = j;
		return i;
	}
	matched_count = 0;
	return -1;
}

/* 
	Recursively seeks for exact match, returns:
		positive number - (file offset of the article) if there was an exact match
		0 - if no match at all can be found

*/
int find_exact_match_counter = 0;
int do_find_exact_match(uint32_t offset, uint16_t* search_str, int str_len) {

	// Safeguard
	if (find_exact_match_counter++ > MAX_EXACT_MATCH_RECURSIVE_CALLS) {
		printf("Internal error, find_exact_match was called %d times", find_exact_match_counter);
		exit(ERR_INTERNAL_ERROR);
	}

	// find all matches, first exact, then "normalized" (case insensitive)
	// recursively call itself
	// Take into account, that there can be only one exact match

	Node node;

	// Read this node
	doseek(offset);
	read_node(node);

	// Number of chars matched
	int matched_count;
	// Idx of the matching child node
	int matched_idx = -1;

	// Doing it in a loop since more than one node can match
	int find_result = find_matching_child(node, search_str, str_len, matched_count, false, matched_idx);
	int exact_match_idx = matched_idx = find_result;

	bool isExactMatch = true;
	while (matched_idx < node.nchildren) {
		// no direct match, looking for normalized comparison
		if (find_result < 0 || matched_count < 1) {
			find_result = find_matching_child(node, search_str, str_len, matched_count, true, matched_idx);
			isExactMatch = false;

			if (find_result > -1 && exact_match_idx == find_result) {
				// already checked this path with exact match
				matched_count = 0;
				matched_idx = find_result;
				continue;
			}
		}

		// can't find matching node
		if (find_result < 0 || matched_count < 1) {
			return 0;
		}

		matched_idx = find_result;

		// matched node offset
		int matched_offset = node.pchildren[matched_idx];

		// found full match
		if (str_len == matched_count) {
			return matched_offset;
		} else if (str_len < matched_count) {
			// matched string longer than oriignal
			find_result = -1;
			continue;
		}
		
		// recursive call, if something is found, return, if not, we still can have a chance with the next match
		if (matched_count > 0) {
			int result = do_find_exact_match(matched_offset, search_str + matched_count, str_len - matched_count);
			if (result != 0) {
				return result;
			} else {
				find_result = -1;
			}
		}

		matched_count = 0;

		// Need to track whether we're looping over exact match (in which case matched_idx needs to be reset)
		// or are already in case insensitive mode
		if (isExactMatch) {
			matched_idx = -1;
			isExactMatch = false;
		}
	}

	// found nothing
	return 0;
}

/**
* Finds nearest match, should be called only if exact match isn't possible.
*/
Node& do_find_best_match(uint32_t offset, uint16_t* search_str, int str_len) {
	Node& node = *(new Node());

	// Find exact match
	doseek(offset);
	read_node(node);

	int i = 0;
	int ret = 0;
	do {
		int matched_count;
		ret = find_matching_child(node, search_str + i, str_len - i, matched_count, false);
		if (ret >= 0) {
			i += matched_count;
			doseek(node.pchildren[ret]);
			read_node(node);
		}
	} while (ret >= 0 && str_len - i >= 0) ;


	// Ensuring to find real, not virtual node
	// virtual node must have at least one child
	while (node.valueArticle == 0) {
		doseek(node.pchildren[0]);
		read_node(node);
	}

	return node;
};

// Converts UTF8 to UTF16
uint16_t* utf8to16(uint8_t* txt, int &len16) {
	// Length of UTF8 version of the search string
	int len = 0;
	while (txt[len] != 0) {
		len++;
	}

	uint16_t* txt16 = (uint16_t*) malloc((len + 1)*2);
	uint16_t* txt16End = txt16;
	ConversionResult res = ConvertUTF8toUTF16((const UTF8**) &txt, (const UTF8*) (txt + len), 
			(UTF16**) &txt16End, (UTF16*) (txt16 + len*2), lenientConversion);

	if (res != conversionOK) {
		printf("Failed to convert input to UTF8");
		exit(ERR_INTERNAL_ERROR);
	}

	// Size of UTF16 version of the search string (in two byte characters)
	len16 = (int) (txt16End - txt16);
	// Set trailing zero
	txt16[len16] = 0;

	return txt16;
};

void dump_article(uint32_t offset) {
	printf("match\n");
	doseek(offset);
	uint8_t article_len[4];
	doread(&article_len, sizeof(article_len));
	int len = article_len[0] + 256*article_len[1] + 256*256*article_len[2] + 256*256*256*article_len[3];
	uint8_t* buf = (uint8_t*) malloc(len + 1);
	doread(buf, len);
	buf[len] = 0;
	printf((char*) buf);
	free(buf);
}

// Finds and dumps exact match, returns false if nothing can be found
bool find_exact_match (uint32_t offset, uint16_t* search_str, int str_len) {
	bool result = false;
	
	int searchResult = do_find_exact_match(offset, search_str, str_len);
	if (searchResult != 0) {
		doseek(searchResult);
		Node node;
		read_node(node);

		if (node.valueArticle != 0) {
			result = true;
			dump_article(node.valueArticle);
		}
	}

	return result;
}


// Dumps WORD_LIST_MAX words, if available
enum SEEK_DIRECTION {PREV, MIDDLE, NEXT};
void dump_word_list(uint32_t offset, SEEK_DIRECTION direction = NEXT, int words_to_seek_back = WORD_LIST_MAX * 2) {
	int words_to_show = WORD_LIST_MAX * 2;
	uint8_t buf[WORD_LIST_BUF_LEN];
	uint32_t max_offset = header.offset_radix;
	uint32_t min_offset = header.offset_word_list;
	uint32_t currentPos = offset;

	printf("list\n");

	// words available after "rewind" operation
	int rewind_words = 0;
	if (direction != NEXT) {
		// need to do one step back
		currentPos--;

		// rewind backward for up to WORD_LIST_MAX words (each word is 2 zeros)
		// since rewinding backward, we need to find extra zero
		int lastZero = -1;
		int len = 0;
		while (rewind_words < words_to_seek_back && currentPos >= min_offset) {
			len = currentPos - WORD_LIST_BUF_LEN >= min_offset ? WORD_LIST_BUF_LEN : currentPos - min_offset;
			lastZero = -1;
			doseek(currentPos - len);	
			doread(buf, len);
			for (uint32_t i = len; i >= 0 && rewind_words < words_to_seek_back; i--) {
				if (buf[i] == 0) {
					lastZero = i;
					rewind_words++;
				}
			}

			if (lastZero < 1) {
				// couldn't find any more matches, exit loop
				break;
			}

			// adjust current position
			currentPos -= (len - lastZero - 1);

		}

		words_to_show = rewind_words;
		if (direction == MIDDLE) {
			words_to_show *= 2 ;
		}

		if (rewind_words > 0 && lastZero > 0) {
			// need to rewind to the next zero
			int i = 0;
			while (buf[lastZero + i] != 0 && i < len) {
				i++;
			}

			currentPos -= i;
		}
	}

	// Reading word list with short translations
	// <word>\0<translation>\0
	//	
	int startingPos = currentPos; // position where we've started to dump the list
	doseek(currentPos);
	int printedWords = 0;
	// where we have seen zero last time
	// not really eof, but end of word list.
	// word list ends where radix starts
	bool eofReached = false;
	do {
		// header.offset is where word list ends and radix begins
		// find max length we can read from word list block
		uint32_t len = currentPos + WORD_LIST_BUF_LEN < max_offset ? WORD_LIST_BUF_LEN : max_offset - currentPos;
		doread(buf, len);
		int lastZero = -1;
		for (uint32_t i = 0; i < len && printedWords < words_to_show; i++) {
			if (buf[i] == 0) {
				buf[i] = printedWords % 2 == 0 ? '\t' : '\n';
				lastZero = i;
				printedWords++;
			}
		}

		if (lastZero < 1) {
			// couldn't find any more matches, exit loop
			break;
		}

		// Ok, read the buffer, let's print what we've read
		// put zero on the last seen zero position
		buf[lastZero] = 0;
		printf((char*) &buf);

		// adjust current position
		currentPos += lastZero + 1;
	} while (printedWords < words_to_show && currentPos < offset);

	printf("\n%d\t%d", startingPos, currentPos);
	// TODO what about the case when no direct match can be found, even for the word from the list?
	// shouldn't word list also contain pointers to particular word translation?

}

// Finds and dumps betst matching words
void find_best_match(uint16_t* search_str, int str_len) {
	// No direct match, looking for best match
	Node& node = do_find_best_match(header.offset_radix, search_str, str_len);
	dump_word_list(node.valueWordList, MIDDLE, WORD_LIST_MAX);
}

//
// Supported commands are:
//	<dict.file> e <word> - find exact match, if nothing found, dump list
//	<dict.file> l <word> - dump list
//  <dict.file> n <offset> - dump next word list starting from <offset>
//  <dict.file> p <offset> - dump previous word list, ending at offset <offset>
//  <dict.file> x <offset> - dump article at address x
//
// Output format:
//	list result
//		"list"
//		<word>\t<short translation>\n
//		<word>\t<short translation>\n
//		...
//		<starting_offset>\t<ending_offset>\n
//
//	exact result
//		"match"\n
//		<translation>
//			
int main(int argc, char* argv[])
{
	// Set console codepage to UTF8 on windows
	#ifdef _MSC_VER
		SetConsoleOutputCP(CP_UTF8);
	#endif


	// Expecting <exec name> <dictionary file> <word>
	if (argc < 4) {
		print_usage();
		return ERR_INVALID_ARGUMENT;
	}

	char* command = (char*) argv[2];

	// dictionary file name, fancy names aren't supported
	char* filename = (char*) argv[1];

	// UTF 16 version of the search string (assuming input is UTF8)
	int search_len;
	uint16_t* searchUTF16 = utf8to16((uint8_t*) argv[3], search_len);
	
	// Open dictionary file
	f = fopen(filename, "rb");
	if (!f) {
		printf("Failed to open file: %s", filename);
		return ERR_FILE_NOT_FOUND;
	}

	// Read header
	doread(&header, sizeof(header));

	// check magic (PRSPDICT ascii)
	int MAGIC[8] = {0x50, 0x52, 0x53, 0x50, 0x44, 0x49, 0x43, 0x54};
	for (int i = 0; i < 8; i++) {
		if (header.magic[i] != MAGIC[i]) {
			printf("Invalid file magic");
			return ERR_INVALID_MAGIC;
		}
	}

	// Check dicitonary version
	if (header.version_lo != 0 || header.version_hi != 1) {
		printf("Unsupported dictionary version: %d.%d", header.version_hi, header.version_lo);
		return ERR_UNSUPPORTED_VERSION;
	}

	switch (command[0]) {
		case 'e':
			//	e <word> - find exact match, if nothing found, dump list
			if (!find_exact_match(header.offset_radix, searchUTF16, search_len)) {
				find_best_match(searchUTF16, search_len);
			}
			break;
		//	l <word> - dump list
		case 'l':
			find_best_match(searchUTF16, search_len);
			break;
		//  n <offset> - dump next word list starting from <offset>
		case 'n':
			dump_word_list(atoi(argv[3]), NEXT);
			break;
		//  p <offset> - dump previous word list, ending at offset <offset>
		case 'p':
			dump_word_list(atoi(argv[3]), PREV);
			break;
		//  x <offset> - dump article at address x
		case 'x':
			dump_article(atoi(argv[3]));
			break;
		default:
			print_usage();
	}

	
	fclose(f);
	return 0;
}
