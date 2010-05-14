/*
 * ydec.c by Jean-Nicolas Jolivet
 */
#include "ydec.h"


char *execute_regex(char *pattern, char *subject, int desired_match)
{
	pcre *re;
	const char *einfo;
	int eoffset;
	int stringcount = 0;
	int ovector[desired_match * 3];
	int match_len;
	const char *match;
	char *match_returned;
	re = pcre_compile(pattern,
					  0, /* default options*/
					  &einfo,
					  &eoffset,
					  NULL);
	if(!re) {
		fprintf(stderr, "%s\n", einfo);
		return NULL;
	}
	
	stringcount = pcre_exec(re, NULL, subject, strlen(subject), 0, 0, ovector, 30);
	match_len = pcre_get_substring(subject, ovector, stringcount, desired_match, &match);
	if(match_len <= 0) {
		/* Problem... bail */
		return NULL;
	}
	// assing to the ret value
	match_returned = (char *)malloc(match_len);
	strncpy(match_returned, match, match_len);
	pcre_free_substring(match);
	return match_returned;
	
}

size_t get_file_size(char *haystack)
{

	size_t result = 0;
	int scanned	= 0;
	char *match;
	
	match = execute_regex("size=(\\d+)", haystack, 1);

	scanned = sscanf(match, "%ld", &result);
	if(scanned != 1)
	{
		fprintf(stderr, "Regex Scanning Error. Probably a badly formatted ybegin line!\n");
		return 0;
	}
	
	// cleanup
	pcre_free_substring(match);
	
	return result;
}


int parse_file(char *path, char **data_head, char **data_tail, char **data, int *data_size)
{
	FILE *fp;
	long fz;
	char *buf;
	
	pcre *re;
	const char *einfo;
	int eoffset;
	int stringcount = 0;
	int ovector[30];
	const char *head = NULL;
	const char *tail = NULL;
	const char *fdata = NULL;
	int head_size, tail_size;
	int datastart, dataend, datalength;
	
	// open the file and read the whole thing...
	
	fp = fopen(path, "r");
	if(!fp) {
		fprintf(stderr, "Couldn't open file!\n");
		return 0;
	}
	fseek(fp, 0L, SEEK_END);
	fz = ftell(fp);
	
	fseek(fp, 0L, SEEK_SET);
	buf = (char *)malloc(fz);
	
	fread(buf, 1, fz, fp);
	
	// try to find the two ranges we need
	re = pcre_compile("(=ybegin[^\r\n]+).+(=yend[^\r\n]+)",
					  (PCRE_CASELESS|PCRE_DOTALL|PCRE_MULTILINE|PCRE_NEWLINE_ANYCRLF),
					  &einfo,
					  &eoffset,
					  NULL);
	
	if(!re) {
		fprintf(stderr, "%s\n", einfo);
		return 0;
	}

	stringcount = pcre_exec(re, NULL, buf, 
							fz, 0, 0, ovector, 30);
	
	
	head_size = pcre_get_substring(buf, ovector, stringcount, 1, &head);
	tail_size = pcre_get_substring(buf, ovector, stringcount, 2, &tail);
	fprintf(stderr, "Head Size: %d, Tail Size: %d\n", head_size, tail_size);
	*data_head = malloc(head_size);
	strncpy(*data_head, head, head_size);
	
	*data_tail = malloc(tail_size);
	strncpy(*data_tail, tail, tail_size);
	
	if(!data_head || !data_tail)
	{
		fprintf(stderr, "Not a yenc file!\n");
		return 0;
	}
	datastart = ovector[3] + 1;
	dataend = ovector[4] -1;
	datalength = dataend - datastart;
	*data_size = datalength;
	
	*data = malloc(datalength);
	strncpy(*data, buf + datastart, datalength);
	
	//printf("Data: %s\n", *data);
	
	free(buf);
	pcre_free_substring(head);
	pcre_free_substring(tail);

	
	return 1;
	
}


int decode(char *data, size_t datasize, char *output_data, int *output_data_length, int intput_data_length)
{

	char decoded[datasize];
	int decoded_bytes = 0;
	int escape = 0;
	Byte byte;
  	int i;
	
	for (i=0; i < intput_data_length; i++) {
		byte = data[i];
		if(escape) {
			byte = (Byte)(byte - 106);
			escape = 0;
		}
		else if(byte == ESC) {
			escape = 1;
			continue;
		}
		else if(byte == CR || byte == LF) {
			continue;
		}
		else {
			byte = (Byte)(byte - 42);
		}
		//printf("%c", byte);
		output_data[decoded_bytes] = byte;
		decoded_bytes++;
		
		
	}
	fprintf(stderr, "Decoded size: %d\n", decoded_bytes);
	*output_data_length = decoded_bytes;
	
	return 1;
}


int main(int argc, char* argv[])
{
	char *data_head	= NULL;	// The data head (ybegin line)
	char *data_tail	= NULL;	// The data tail (yend line)
	char *data = NULL;	// The data itself
	char *output_data = NULL;	// The decoded data
	int parse_result = 0;	// the result of the parsing operation
	int output_data_length = 0;	// size of the outputed data...
	int intput_data_length = 0;
	size_t file_size;
	char *file_name = NULL;
	FILE *fp;
	DWORD checksum;
	
	// needs at least 1 argument!
	if (argc < 2) {
		fprintf(stderr, "Too few arguments! usage: %s somefile.txt\n", argv[0]);
		return 1;
	}
	

	parse_result = parse_file(argv[1], &data_head, &data_tail, &data, &intput_data_length);
	
	if(parse_result != 1)
		return parse_result;
	
	
	if (!data_head || !data_tail || !data)
	{
		fprintf(stderr, "Not a YEnc file!\n");
		return 1;
	}
	
	
	file_size = get_file_size(data_head);
	file_name = execute_regex("name=(\\S+)", data_head, 1);
	if(!file_name)
		file_name = "output.txt";
	
	output_data = (char *)malloc(file_size);
	decode(data, file_size,output_data, &output_data_length, intput_data_length);
	
	fp = fopen(file_name, "wb");
	fwrite(output_data, 1, output_data_length, fp);
	
	checksum = crc32buf(output_data, output_data_length);
	fprintf(stderr, "Checksum: %x\n", checksum);
	
	

	fclose(fp);
	
	free(file_name);
	free(data_head);
	free(data_tail);
	free(data);
	
	
	return 0;
	
}
