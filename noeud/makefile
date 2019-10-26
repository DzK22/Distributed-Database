CC = gcc -g
CFLAGS = -Wall -Werror -Wextra -pthread
EXEC = main
SRC_DIR = src/
OBJ_DIR = obj/
HD_DIR = headers/
DOC_DIR = doc/

$(EXEC): $(OBJ_DIR)main.o $(OBJ_DIR)noeud.o ../common/obj/sck.o ../common/obj/datagram.o
	$(CC) -o $(EXEC) $^ $(CFLAGS)

$(OBJ_DIR)$(EXEC).o: $(SRC_DIR)$(EXEC).c $(HD_DIR)noeud.h
	$(CC) -c $< $(CFLAGS)
	mkdir -p $(OBJ_DIR)
	mv $(EXEC).o $(OBJ_DIR)

$(OBJ_DIR)noeud.o: $(SRC_DIR)noeud.c $(HD_DIR)noeud.h
	$(CC) -c $< $(CFLAGS)
	mkdir -p $(OBJ_DIR)
	mv noeud.o $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(EXEC) $(DOC_DIR)
