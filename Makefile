# FIXME this makefile is crappy - clean it up!

# Compiler
CC   = gcc
OPTS = -std=c99 -pedantic -g

# Project name
PROJECT = blarb

SRC_DIR = 'src'
OBJ_DIR = 'obj'

SRCS = $(shell find $(SRC_DIR) -name '*.c')
DIRS = $(shell find $(SRC_DIR) -type d | sed 's/$(SRC_DIR)/./g' ) 
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Targets
$(PROJECT): buildrepo $(OBJS)
	$(CC) $(OPTS) $(OBJS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(OPTS) -c $< $(INCS) -o $@

clean:
	rm $(PROJECT) $(OBJ_DIR) -Rf

buildrepo:
	mkdir -p $(OBJ_DIR)
	for dir in $(DIRS); do mkdir -p $(OBJ_DIR)/$$dir; done

