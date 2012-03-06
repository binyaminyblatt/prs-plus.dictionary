// Radix.cpp 

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

#define ERR_FILE_NOT_FOUND -1
#define ERR_INTERNAL_ERROR -2
#define ERR_INVALID_ARGUMENT -3
#define ERR_UNSUPPORTED_VERSION -4

#define OFFSET_NCHILDREN 6

#pragma pack(1)

FILE* f;

struct Header {
	uint8_t magic[8];
	uint16_t size;
	uint8_t version_lo;
	uint8_t version_hi;
	uint32_t offset;
	uint8_t junk[1008];
};

struct Node {
	uint16_t len; // length of the structure
	uint32_t value; // pointer to the article (offset in file)
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
* Exits program with ERR_INTERNAL_ERROR in case of errors.
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
int find_match(Node node, uint16_t* search_str, int str_len, int& matched_count) {
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

int main(int argc, char* argv[])
{
	if (argc < 3) {
		print_usage();
		return ERR_INVALID_ARGUMENT;
	}

	char* filename = (char*) argv[1];
	// TODO unescape input string
	uint8_t* searchUTF8 = (uint8_t*) argv[2];

	int search_len = 0;
	while(searchUTF8[search_len] != 0) {
		search_len++;
	}

	uint16_t* searchUTF16 = (uint16_t*) malloc((search_len + 1)*2);
	uint8_t* searchUTF8End = searchUTF8;
	uint16_t* searchUTF16End = searchUTF16;

	ConversionResult res = ConvertUTF8toUTF16((const UTF8**) &searchUTF8End, (const UTF8*) (searchUTF8 + search_len), (UTF16**) &searchUTF16End, (UTF16*) (searchUTF16 + search_len*2 /* TODO need *2 multiplier? */), lenientConversion);
//	ConversionResult res = ConvertUTF16toUTF8((const UTF16**) &name,(const UTF16*) (name + len), (UTF8**) &pbuf0, (UTF8*) pbuf0 + utf8Len, lenientConversion);
	if (res != conversionOK) {
		printf("Failed to convert input to UTF8");
		return -1;
	}
	search_len = (int) (searchUTF16End - searchUTF16);
	// Set trailing zero
	searchUTF16[search_len] = 0;
	
	printf("dict filename is '%s' search string is '%s' search string length is '%d'\n", filename, searchUTF8, search_len);
	// Open dictionary file
	f = fopen(filename, "rb");
	if (!f) {
		printf("Failed to open file: %s", filename);
		return ERR_FILE_NOT_FOUND;
	}

	// Read header
	Header header;
	doread(&header, sizeof(header));

	// Check dicitonary version
	if (header.version_lo != 0 || header.version_hi != 1) {
		printf("Unsupported dictionary version: %d.%d", header.version_hi, header.version_lo);
		return ERR_UNSUPPORTED_VERSION;
	}
	// TODO check magic

	// Move to indices
	doseek(header.offset);

	Node node;
	read_node(node);

	int n_disk_accesses = 1;
 	int nfound = 0;
	while (nfound < search_len) {
		int ncount;
		int result = find_match(node, searchUTF16 + nfound, search_len - nfound, ncount);
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
	if (node.value != 0 && nfound == search_len) {
		printf("Found %d chars match, disk accessed %d times, offset is %d\n", nfound, n_disk_accesses, node.value);
		doseek(node.value);
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
		printf("String '%s' wasn't found\n", searchUTF16);
	}

	
	fclose(f);
	return 0;
}