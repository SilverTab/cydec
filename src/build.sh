clang -Wall -std=c99 -o ../bin/ydec -ISource \
	ydec.c \
	crc/crc_32.c \
-lpcre