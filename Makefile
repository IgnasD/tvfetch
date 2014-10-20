all:
	gcc -Wall -O2 -s `xml2-config --cflags` src/main.c -lcurl -lxml2 -lpcre -o bin/tvfetch
