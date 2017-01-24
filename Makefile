# Compiler
CC   = gcc
OPTS = -std=c11 -pedantic -g -O3

# Project name
PROJECT = blarb

SRC_DIR = 'src'
OBJ_DIR = 'obj'

SRCS = $(shell find $(SRC_DIR) -name '*.c')

# Targets
$(PROJECT): buildrepo compileScanner
	$(CC) $(OPTS) $(SRCS) -o $@

clean:
	rm $(PROJECT) $(OBJ_DIR) -Rf

compileScanner:
	lex -o $(OBJ_DIR)/blarb.yy.c $(SRC_DIR)/blarb.lex

buildrepo:
	mkdir -p $(OBJ_DIR)

