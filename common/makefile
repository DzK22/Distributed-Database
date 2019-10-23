CC = gcc -g
CFLAGS = -Wall -Werror -Wextra
SRC_DIR = src/
OBJ_DIR = obj/
HD_DIR = headers/
DOC_DIR = doc/

all: $(OBJ_DIR)datagram.o $(OBJ_DIR)sck.o

$(OBJ_DIR)datagram.o: $(SRC_DIR)datagram.c $(HD_DIR)datagram.h
	$(CC) -c $< $(CFLAGS)
	mkdir -p $(OBJ_DIR)
	mv datagram.o $(OBJ_DIR)

$(OBJ_DIR)sck.o: $(SRC_DIR)sck.c $(HD_DIR)sck.h
	$(CC) -c $< $(CFLAGS)
	mkdir -p $(OBJ_DIR)
	mv sck.o $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(DOC_DIR)
