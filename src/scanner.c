#include <stdint.h>

#include "main.h"
#include "vm.h"
#include "../obj/blarb.yy.c"

void BlarbVM_addLabelPointer(BlarbVM *vm, char *name, int line) {
    LabelPointer *lab;
    HASH_FIND_STR(vm->labelPointers, name, lab);
    if (lab) {
        fprintf(stderr, "Duplicate label '#%s', found in %s on line %d\n",
                name, yyfilename, yylineno);
        exit(1);
    }

    LabelPointer *newLabel = malloc(sizeof(LabelPointer));
    strncpy(newLabel->name, name, sizeof(newLabel->name));
    newLabel->line = line;

    HASH_ADD_STR(vm->labelPointers, name, newLabel);
}

// Parses a char in a string or character literal
char parseChar(char **str) {
    char c = **str;
    // Handle escaped characters
    if (**str == '\\') {
        char escapedChar = *(*str + 1);
        switch (escapedChar) {
        case 'n':
            (*str)++;
            c = '\n';
            break;
        case 't':
            (*str)++;
            c = '\t';
            break;
        case '\\':
            (*str)++;
            c = '\\';
            break;
        case '\'':
            (*str)++;
            c = '\'';
            break;
        case '\"':
            (*str)++;
            c = '\"';
            break;
        default:
            fprintf(stderr, "Invalid character escape: %c\n", escapedChar);
        }
    }
    (*str)++;
    return c;
}

void addStringToToken(token *t, char *str, size_t len) {
    t->str = malloc(len + 1);
    t->str[len] = '\0';
    strncpy(t->str, str, len);
}

void addStringLiteralToToken(token *t, char *str) {
    str++; // Cut out first double quote
    *(strrchr(str, '\"')) = 0; // Cut out second double quote

    t->str = malloc(strlen(str) + 1);

    int i;
    char *itr;
    for (i = 0, itr = str; *itr; i++) {
        t->str[i] = parseChar(&itr);
    }
    t->str[i] = '\0';
}

// Scans a line of tokens using yylex
token * BlarbVM_scanLine(BlarbVM *vm) {
    int tokenCount = 0;
    // 1024 tokens should be way more than enough.
    token *line = malloc(sizeof(token) * 1024);
    token_t tokenType;

    int label_call_present = 0;
    int newline_present = 0;

    while ((tokenType = yylex())) {
        token *t = &line[tokenCount++];
        // In general, we just care about the type.
        t->type = tokenType;

        if (tokenType == NEWLINE) {
            newline_present = 1;
            break;
        // No tokens (except newlines) are allowed after a function call
        } else if (label_call_present) {
            fprintf(stderr, "Parse error: "
                    "No tokens are allowed after calls to labels.\n"
                    "Error on line %d in %s\n",
                    yylineno, yyfilename);
            exit(1);
        } else if (tokenType == INTEGER) {
            t->val = strtoull(yytext, 0, 10);
        } else if (tokenType == LABEL_CALL) {
            addStringToToken(t, yytext, strlen(yytext));
            label_call_present = 1;
        } else if (tokenType == LABEL) {
            // Exclude the hash
            addStringToToken(t, yytext + 1, strlen(yytext) - 1);
            BlarbVM_addLabelPointer(vm, t->str, vm->lineCount);
        } else if (tokenType == STR) {
            addStringLiteralToToken(t, yytext);
        } else if (tokenType == CHR) {
            t->type = INTEGER;
            char *s = yytext + 1;
            t->val = parseChar(&s);
        }
    }

    // If this is the EOF (no more tokens)
    if (tokenCount == 0) {
        free(line);
        return 0;
    }

    // In case we hit an EOF on a valid line (we use newlines for terminators)
    if ( ! newline_present) {
        token *t = &line[tokenCount++];
        t->type = NEWLINE;
    }

    // Resize the memory chunk so we don't waste memory on small lines
    return realloc(line, sizeof(token) * tokenCount);
}

void BlarbVM_addLineDebugInfo(BlarbVM *vm, char *fileName) {
    vm->linesDebug = realloc(vm->linesDebug,
                             sizeof(LineDebugInfo) * vm->lineCount);

    LineDebugInfo *info = &(vm->linesDebug[vm->lineCount - 1]);
    info->fileName = fileName;
    info->line = yylineno - 1;
}

/*
 * Note: I know I know, I should really be using a set.
 * ... but a lookup table is good enough for now.
 * I don't want to worry about premature optimizations :)
 */
int BlarbVM_shouldLoadFileName(BlarbVM *vm, char *fileName) {
    size_t fileNameLen = strlen(fileName);

    // Ignore duplicates
    for (size_t i = 0; i < vm->loadedFileNameCount; i++) {
        if (strncmp(fileName, vm->loadedFileNames[i], fileNameLen) == 0) {
            return 0;
        }
    }

    return 1;
}

char * BlarbVM_addFileNameToLoadedFiles(BlarbVM *vm, char *fileName) {
    size_t fileNameLen = strlen(fileName);

    vm->loadedFileNameCount++;
    vm->loadedFileNames = realloc(vm->loadedFileNames, vm->loadedFileNameCount);

    char **new  = &vm->loadedFileNames[vm->loadedFileNameCount - 1];
    *new = malloc(fileNameLen);
    strncpy(*new, fileName, fileNameLen + 1);

    char *savedFileName = *new;
    savedFileName[fileNameLen] = '\0';

    return savedFileName;
}

void BlarbVM_loadFile(BlarbVM *vm, char *fileName) {
    if ( ! BlarbVM_shouldLoadFileName(vm, fileName)) {
        return;
    }

    // Stash away the file name safely in memory
    fileName = BlarbVM_addFileNameToLoadedFiles(vm, fileName);

	FILE *fp = fopen(fileName, "r");
	if ( ! fp) {
		fprintf(stderr, "Failed to open '%s'\n", fileName);
		perror("fopen");
		terminateVM();
	}
    yyin = fp;
    yyfilename = fileName;

	token *line;
    while ((line = BlarbVM_scanLine(vm))) {
        BlarbVM_addLine(vm, line);
        BlarbVM_addLineDebugInfo(vm, fileName);
    }

    yyin = stdin;
	fclose(fp);
}

void BlarbVM_addLine(BlarbVM *vm, token *line) {
	if (ISPOWEROF2(vm->lineCount) || vm->lineCount == 0) {
		// Double the size when space runs out - amortized time is O(1)
		vm->lines = vm->lineCount == 0
			? malloc(sizeof(token_t *) * 1)
			: realloc(vm->lines, sizeof(token_t *) * vm->lineCount * 2);
	}
	vm->lines[vm->lineCount++] = line;
}

