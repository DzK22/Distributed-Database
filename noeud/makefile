CC = gcc -g
CFLAGS = -Wall -Werror -Wextra
EXEC = main
SRC_DIR = src/
OBJ_DIR = obj/
HD_DIR = headers/
DOC_DIR = doc/

$(EXEC): $(OBJ_DIR)main.o $(OBJ_DIR)noeuds.o
	$(CC) -o $(EXEC) $^ $(CFLAGS)

$(OBJ_DIR)$(EXEC).o: $(SRC_DIR)$(EXEC).c $(HD_DIR)noeuds.h
	$(CC) -c $< $(CFLAGS)
	mkdir -p $(OBJ_DIR)
	mv $(EXEC).o $(OBJ_DIR)

$(OBJ_DIR)noeuds.o: $(SRC_DIR)noeuds.c $(HD_DIR)noeuds.h
	$(CC) -c $< $(CFLAGS)
	mkdir -p $(OBJ_DIR)
	mv noeuds.o $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(EXEC) $(DOC_DIR)
