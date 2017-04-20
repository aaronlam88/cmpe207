# Make file
# all source code should be in src folder
# all objects will be place in obj folder
# link necessary objects to create exe program

# CFLAGS = $(shell mysql_config --cflags)
INCLUDE = -include $(shell mysql_config --variable=pkgincludedir).h
CFLAGS = $(shell mysql_config --cflags)
LIBS = $(shell mysql_config --libs)
CC = gcc -Wall

#================== Source Files ==================
ERREXIT_SRC = src/errexit.c 
HELPER_SRC  = src/mysql_connect.c
SERVERS_SRC = src/passivesock.c
#==================================================

#================== Object Files ==================
HELPER_OBJ = obj/mysql_connect.o
SERVER_OBJ = obj/errexit.o obj/passivesock.o obj/tcpTPRServer.o
#==================================================

#================ Executable Files ================
PROGRAM =  tcpTPRServer
#==================================================

all: $(PROGRAM)

obj/errexit.o: src/errexit.c
	$(CC) -c src/errexit.c -o obj/errexit.o
obj/passivesock.o: src/passivesock.c
	$(CC) -c src/passivesock.c -o obj/passivesock.o

obj/mysql_connect.o: src/mysql_connect.c src/mysql_connect.h
	$(CC) -c $(INCLUDE) $(CFLAGS) $(LIBS) src/mysql_connect.c -o obj/mysql_connect.o

obj/tcpTPRServer.o: src/tcpTPRServer.c obj/mysql_connect.o
	$(CC) -c $(INCLUDE) $(CFLAGS) $(LIBS) -lpthread src/tcpTPRServer.c -o obj/tcpTPRServer.o 

tcpTPRServer: $(HELPER_OBJ) $(SERVER_OBJ)
	gcc $(HELPER_OBJ) $(SERVER_OBJ) -o Server -lpthread $(CFLAGS) $(LIBS)

clean:
	@rm -v obj/*.o $(PROGRAM)

