CC = gcc -g
CFLAGS = -Wall -Werror -Wextra
EXEC = main
SRC_DIR = src/
OBJ_DIR = obj/
HD_DIR = headers/

$(EXEC): $(OBJ_DIR)main.o $(OBJ_DIR)clients.o
	$(CC) -o $(EXEC) $^ $(CFLAGS)

$(OBJ_DIR)main.o: $(SRC_DIR)main.c $(HD_DIR)clients.h
	$(CC) -c $< $(CFLAGS)
	mv main.o $(OBJ_DIR)

$(OBJ_DIR)clients.o: $(SRC_DIR)clients.c $(HD_DIR)clients.h
	$(CC) -c $< $(CFLAGS)
	mv clients.o $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)* main doc/
