CC = gcc -g
CFLAGS = -Wall -Werror -Wextra
EXEC = main
SRC_DIR = src/
OBJ_DIR = obj/
HD_DIR = headers/
DOC_DIR = doc/

$(EXEC): $(OBJ_DIR)$(EXEC).o $(OBJ_DIR)relais.o
	$(CC) -o $(EXEC) $^ $(CFLAGS)

$(OBJ_DIR)$(EXEC).o: $(SRC_DIR)$(EXEC).c $(HD_DIR)relais.h
	$(CC) -c $< $(CFLAGS)
	mkdir -p $(OBJ_DIR)
	mv $(EXEC).o $(OBJ_DIR)

$(OBJ_DIR)relais.o: $(SRC_DIR)relais.c $(HD_DIR)relais.h
	$(CC) -c $< $(CFLAGS)
	mkdir -p $(OBJ_DIR)
	mv relais.o $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(EXEC) $(DOC_DIR)
