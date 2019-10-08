CC = gcc -g
CFLAGS = -Wall -Werror -Wextra
EXEC = main
SRC_DIR = src/
OBJ_DIR = obj/
HD_DIR = headers/

$(EXEC): $(OBJ_DIR)main.o $(OBJ_DIR)noeuds.o
	$(CC) -o $(EXEC) $^ $(CFLAGS)

$(OBJ_DIR)main.o: $(SRC_DIR)main.c $(HD_DIR)noeuds.h
	$(CC) -c $< $(CFLAGS)
	mv main.o $(OBJ_DIR)

$(OBJ_DIR)noeuds.o: $(SRC_DIR)noeuds.c $(HD_DIR)noeuds.h
	$(CC) -c $< $(CFLAGS)
	mv noeuds.o $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)* main
