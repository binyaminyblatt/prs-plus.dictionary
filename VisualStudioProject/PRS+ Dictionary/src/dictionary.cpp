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

#pragma pack(1)

FILE* f;

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
	uint32_t pchildren[1024]; // pointers to child nodes (offset in file)
	uint16_t children_names [64*1024]; // zero terminated UTF-16 names
};
int size_of_node = sizeof(Node) - sizeof(uint32_t[1024]) - sizeof(uint16_t[64*1024]);
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
	doread(&(node.pchildren), pchildren_len);
	doread(&(node.children_names), node.len - size_of_node - pchildren_len);
}

/**
* Finds matching node in the list referenced by <node>. Returns -1 if no matching node were found.
*/
int find_matching_child(Node node, uint16_t* search_str, int str_len, int& matched_count) {
	uint16_t* pchildren = &(node.children_names[0]);
	for (int i = 0, n = node.nchildren; i < n; i++) {
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
			// TODO case sensitivity
			if (pchildren[j] != search_str[j]) {
				mismatch = true;
				break;
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
	Recursively seeks best match, returns:
		positive number - (file offset of the article) if there was an exact match
		negative number - (negative of file offset of the word list) if string can be matched only partially
		0 - if no match at all can be found (TODO is this possible?)

*/
int find_match(uint32_t offset, uint16_t* search_str, int str_len) {
	// TODO
	return 0;
}

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

	// Move to indices
	doseek(header.offset);

	Node node;
	// Read root node
	read_node(node);

	int n_disk_accesses = 1;
	// number of matching chars
 	int nfound = 0;
	while (nfound < search_len) {
		int ncount;
		int result = find_matching_child(node, searchUTF16 + nfound, search_len - nfound, ncount);
		if (result < 0) {
			break;
		}
		nfound += ncount;
		doseek(node.pchildren[result]);
		read_node(node);
		n_disk_accesses++;
	}
	
	// TODO find list of matching words
	
	// Dumping article, if exact match
	if (node.valueArticle != 0 && nfound == search_len) {
		printf("Found %d chars match, disk accessed %d times, offset is %d\n", nfound, n_disk_accesses, node.valueArticle);
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
	} else {
		// TODO dump word list
		printf("String '%s' wasn't found\n", searchUTF8);
	}

	
	fclose(f);
	return 0;
}