CC = gcc
CPPFLAGS = -DDEBUG -DLOG_LEVEL=LOG_DEBUG -I. -I.. -I../..
CFLAGS = -Wall -Wextra -g
LDLIBS = -laio

.PHONY: build clean

build: aws

aws: aws.o sock_util.o http-parser/http_parser.o

aws.o: aws.c sock_util.h debug.h util.h

sock_util.o: sock_util.c sock_util.h debug.h util.h

http-parser/http_parser.o: http-parser/http_parser.c http-parser/http_parser.h
	make -C http-parser/ http_parser.o

clean:
	-rm -f *~
	-rm -f *.o
	-rm -f http-parser/http_parser.o
	-rm -f aws
