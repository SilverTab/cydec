#include <stdio.h>		/* for file io 			*/
#include <string.h>		/* for strncpy etc. 		*/
#include <stdlib.h>		/* for malloc, free etc. 	*/
#include <pcre.h>		/* for regular expressions 	*/
#include "crc/crc.h"		/* for crc 32 checksum 		*/

#define CR		0x0d
#define LF		0x0a
#define ESC		0x3d

typedef unsigned char Byte;

char *execute_regex(char *pattern, char *subject, int desired_match);
size_t get_file_size(char *haystack);
int parse_file(char *path, char **data_head, char **data_tail, char **data, int *data_size);
int decode(char *data, size_t datasize, char *output_data, int *output_data_length, int intput_data_length);