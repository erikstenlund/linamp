PROGRAM = linamp
PROGRAM_FILES = linamp.c

CC = gcc
CFLAGS = -Wall $(shell pkg-config --cflags gstreamer-1.0)
LIBS = $(shell pkg-config --libs gstreamer-1.0)


linamp: linamp.c
	$(CC) $(PROGRAM_FILES) $(CFLAGS) $(LIBS) -o linamp

