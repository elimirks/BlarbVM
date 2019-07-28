# Compiler
CC   = gcc
OPTS = -std=gnu11 -pedantic -g -O3

# Project name
PROJECT = blarb

SRC_DIR = 'src'
OBJ_DIR = 'obj'

SRCS = src/debugger.c src/main.c src/scanner.c src/vm.c

ifeq ($(BLARB_LIBRARY_PATH),)
BLARB_LIBRARY_PATH = $(shell pwd)/library
endif

OPTS += -DBLARB_LIBRARY_PATH="\"$(BLARB_LIBRARY_PATH)\""

OS = $(shell uname -s)
ifeq ($(OS),Darwin)
  SRCS += src/syscall_macos.c
else ifeq ($(OS),Linux)
  SRCS += src/syscall_linux.c
endif

# Targets
$(PROJECT): buildrepo compileScanner
	$(CC) $(OPTS) $(SRCS) -o $@

clean:
	rm $(PROJECT) $(OBJ_DIR) -Rf

compileScanner:
	lex -o $(OBJ_DIR)/blarb.yy.c $(SRC_DIR)/blarb.lex

buildrepo:
	mkdir -p $(OBJ_DIR)

