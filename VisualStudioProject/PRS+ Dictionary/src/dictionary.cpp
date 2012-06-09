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
 *  	version lo (uint8) 
 *  	version hi (uint8)
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

#pragma pack(1)

// Globals
FILE* f;
int n_disk_accesses = 1;

// Dictionary file header
struct Header {
	uint8_t magic[8];
	uint16_t size;
	uint8_t version_lo;
	uint8_t version_hi;
	uint32_t offset;
	uint8_t junk[1008];
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
int size_of_node = sizeof(Node) - sizeof(uint32_t*) - sizeof(uint16_t*) - 2 * (sizeof(int));
int size_of_pchildren = sizeof(uint32_t[1024]);

void print_usage() {
	printf("Usage:\n\t<program> <prspdic file> <search string>");
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
int find_matching_child (Node node, uint16_t* search_str, int str_len, int& matched_count, bool normalize = false, int starting_idx = 0) {
	uint16_t* pchildren = &(node.children_names[0]);
	for (int i = starting_idx, n = node.nchildren; i < n; i++) {
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
		if (mismatch) {
			// Skip to next 0
			while (*pchildren++ != 0);
			continue;
		}

		matched_count = j;
		return i;
	}
	return -1;
}

/* 
	Recursively seeks for exact match, returns:
		positive number - (file offset of the article) if there was an exact match
		0 - if no match at all can be found

*/
int find_exact_match(uint32_t offset, uint16_t* search_str, int str_len) {
	// FIXME add sanity check with max calls counter
	Node node;

	// Find exact match
	doseek(offset);
	read_node(node);

	int matched_count;
	int matched_idx = 0;

	// Doing it in a loop since more than one node can match
	int find_result = find_matching_child(node, search_str, str_len, matched_count, false, matched_idx);
	while (matched_idx < node.nchildren) {
		// no direct match, looking for normalized comparison
		if (find_result < 0) {			
			find_result = find_matching_child(node, search_str, str_len, matched_count, true, matched_idx);
		}

		// can't find matching node
		if (find_result < 0) {
			return 0;
		}

		matched_idx = find_result;

		// matched node offset
		int matched_offset = node.pchildren[matched_idx];

		// found full match
		if (str_len <= matched_count) {
			return matched_offset;
		}
		
		// recursive call, if something is found, return, if not, we still can have a chance with the next match
		if (matched_count > 0) {
			int result = find_exact_match(matched_offset, search_str + matched_count, str_len - matched_count);
			if (result != NULL) {
				return result;
			}
		}

		matched_count = 0;
	}

	// found nothing
	return 0;
}

/**
* Finds nearest match, should be called only if exact match isn't possible.
*/
Node& find_best_match(uint32_t offset, uint16_t* search_str, int str_len) {
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

int main(int argc, char* argv[])
{
	// Expecting <exec name> <dictionary file> <word>
	if (argc < 3) {
		print_usage();
		return ERR_INVALID_ARGUMENT;
	}

	// dictionary file name, fancy names aren't supported
	char* filename = (char*) argv[1];

	// UTF8 version of the search string
	uint8_t* searchUTF8 = (uint8_t*) argv[2];

	// Length of UTF8 version of the search string
	int searchUTF8Len = 0;
	while (searchUTF8[searchUTF8Len] != 0) {
		searchUTF8Len++;
	}

	// Need to convert UTF8 to UTF16 (to simplify lookup procedure)
	// multi char symbols are ignored
	uint16_t* searchUTF16 = (uint16_t*) malloc((searchUTF8Len + 1)*2);
	uint8_t* searchUTF8End = searchUTF8;
	uint16_t* searchUTF16End = searchUTF16;
	ConversionResult res = ConvertUTF8toUTF16((const UTF8**) &searchUTF8End, (const UTF8*) (searchUTF8 + searchUTF8Len), 
			(UTF16**) &searchUTF16End, (UTF16*) (searchUTF16 + searchUTF8Len*2), lenientConversion);
	if (res != conversionOK) {
		printf("Failed to convert input to UTF8");
		return ERR_INTERNAL_ERROR;
	}

	// Size of UTF16 version of the search string (in two byte characters)
	int search_len = (int) (searchUTF16End - searchUTF16);
	// Set trailing zero
	searchUTF16[search_len] = 0;
	
	// Open dictionary file
	f = fopen(filename, "rb");
	if (!f) {
		printf("Failed to open file: %s", filename);
		return ERR_FILE_NOT_FOUND;
	}

	// Read header
	Header header;
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

	// New method of finding exact or nearest match
	bool foundMatch = false;
	int searchResult = find_exact_match(header.offset, searchUTF16, search_len);
	if (searchResult != NULL) {
		doseek(searchResult);
		Node node;
		read_node(node);

		if (node.valueArticle != 0) {
			foundMatch = true;
			printf("Found exact match, disk accessed %d times, offset is %d\n", n_disk_accesses, node.valueArticle);
			doseek(node.valueArticle);
			uint8_t article_len[4];
			doread(&article_len, sizeof(article_len));
			int len = article_len[0] + 256*article_len[1] + 256*256*article_len[2] + 256*256*256*article_len[3];
			printf("Article size: %d\n", len);
			uint8_t* buf = (uint8_t*) malloc(len + 1);
			doread(buf, len);
			buf[len] = 0;
			printf("Article is:\n%s", buf);
			free(buf);
		}
	}
	
	if (!foundMatch) {
		// No direct match, looking for best match
		Node& node = find_best_match(header.offset, searchUTF16, search_len);

		// Reading word list with short translations
		// <word>\0<translation>\0
		//
		uint32_t currentPos = node.valueWordList;
		doseek(currentPos);
		int printedWords = 0;
		// where we have seen zero last time
		// not really eof, but end of word list.
		// word list ends where radix starts
		bool eofReached = false;
		uint8_t buf[WORD_LIST_BUF_LEN];
		do {
			// header.offset is where word list ends and radix begins
			// find max length we can read from word list block
			uint32_t len = currentPos + WORD_LIST_BUF_LEN < header.offset ? WORD_LIST_BUF_LEN : header.offset - currentPos;
			doread(buf, len);
			int lastZero = -1;
			for (uint32_t i = 0; i < len && printedWords < WORD_LIST_MAX*2; i++) {
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
		} while (printedWords < WORD_LIST_MAX*2 && currentPos < header.offset);

		// TODO output current position to be able to scroll further / back
		// TODO what about the case when no direct match can be found, even for the word from the list?
		// shouldn't word list also contain pointers to particular word translation?
	}
	
	fclose(f);
	return 0;
}