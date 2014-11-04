all:
	gcc -Wall -O2 -s `xml2-config --cflags` src/*.c -lcurl -lxml2 -lpcre -o bin/tvfetch
