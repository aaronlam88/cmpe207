# CFLAGS = $(shell mysql_config --cflags)
INCLUDE = -include $(shell mysql_config --variable=pkgincludedir).h
CFLAGS = $(shell mysql_config --cflags)
LIBS = $(shell mysql_config --libs)
CC = gcc -Wall

all:
	$(CC)  $(INCLUDE) $(CFLAGS) -o connect connect.c $(LIBS)