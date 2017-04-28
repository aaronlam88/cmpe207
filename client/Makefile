# Make file
# all source code should be in src folder
# all objects will be place in obj folder
# link necessary objects to create exe program

CC = gcc -Wall

#================== Source Files ==================
ERREXIT_SRC = src/errexit.c 
CLIENT_SRC = src/connectsock.c src/client.c
#==================================================

#================== Object Files ==================
CLIENT_OBJ = obj/errexit.o obj/connectsock.o obj/client.o obj/fileClient.o obj/fileServer.o
#==================================================

#================ Executable Files ================
PROGRAM =  client
#==================================================

all: $(PROGRAM)

obj/%.o: src/%.c
	@gcc -Wall -c $< -o $@

client: $(CLIENT_OBJ)
	$(CC) $(CLIENT_OBJ) -o client
clean:
	@rm -v obj/*.o $(PROGRAM)
