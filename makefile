all: chatclient chatserver

chatclient: chatclient.c chatlinker.c
	gcc -g -w chatclient.c  chatlinker.c -o chatclient

chatserver: chatserver.c chatlinker.c
	gcc -g chatserver.c  chatlinker.c  -o chatserver
.PHONY:clean
clean:
	rm -f chatclient chatserver
