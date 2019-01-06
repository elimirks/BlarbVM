#include <stdint.h>
#include <stdlib.h>
// FIXME: This breaks MacOS support (which wasn't that great anyways)
#include <linux/limits.h>

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
    strcpy(newLabel->name, name);
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

token * BlarbVM_optimizeLine(token *line, BlarbVM_WORD *tokenCount) {
    // The new line is _at most_ the same length as the old line
    token *newLine = malloc(sizeof(token) * *tokenCount);
    BlarbVM_WORD newLineTokenCount = 0;

    // These optimizations combine integer-push operations with other primitives

    for (BlarbVM_WORD i = 0; i < *tokenCount; i++) {
        token *optToken = &newLine[newLineTokenCount++];

        if (*tokenCount >= i + 1 && line[i].type == INTEGER) {
            if (line[i + 1].type == STACK_POP) {
                optToken->type = EXPLICIT_STACK_POP;
                optToken->val  = line[i].val;
                i++;
                continue;
            } else if (line[i + 1].type == REG_GET) {
                optToken->type = EXPLICIT_REG_GET;
                optToken->val  = line[i].val;
                i++;
                continue;
            } else if (line[i + 1].type == CONDITION) {
                optToken->type = EXPLICIT_CONDITION;
                optToken->val  = line[i].val;
                i++;
                continue;
            }
        }

        if (*tokenCount >= i + 2 &&
            line[i].type == INTEGER &&
            line[i + 1].type == INTEGER) {

            if (line[i + 2].type == NAND) {
                optToken->type    = EXPLICIT_NAND;
                optToken->vals[0] = line[i].val;
                optToken->vals[1] = line[i + 1].val;
                i += 2;
                continue;
            }
        }

        *optToken = line[i];
    }

    free(line);
    *tokenCount = newLineTokenCount;
    return newLine;
}

// Scans a line of tokens using yylex
token * BlarbVM_scanLine(BlarbVM *vm) {
    BlarbVM_WORD tokenCount = 0;
    // 1024 tokens *should* be way more than enough.
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
        return NULL;
    }

    // In case we hit an EOF on a valid line (we use newlines for terminators)
    if ( ! newline_present) {
        token *t = &line[tokenCount++];
        t->type = NEWLINE;
    }

    line = BlarbVM_optimizeLine(line, &tokenCount);

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

void BlarbVM_addFileNameToLoadedFiles(BlarbVM *vm, char *fileName) {
    vm->loadedFileNameCount++;
    vm->loadedFileNames = realloc(vm->loadedFileNames,
                                  sizeof(char*) * vm->loadedFileNameCount);
    vm->loadedFileNames[vm->loadedFileNameCount - 1] = fileName;
}

char * BlarbVM_resolveAndAllocFilePath(char *fileName) {
    char resolvedPath[PATH_MAX + 1];

    if (realpath(fileName, resolvedPath) == NULL) {
        perror("realpath");
    }

    size_t pathLength = strlen(resolvedPath);
    char *newFilePath = malloc(pathLength + 1);
    strcpy(newFilePath, resolvedPath);
    newFilePath[pathLength] = '\0';

    return newFilePath;
}

void BlarbVM_loadFile(BlarbVM *vm, char *fileName) {
    char *filePath = BlarbVM_resolveAndAllocFilePath(fileName);

    if ( ! BlarbVM_shouldLoadFileName(vm, filePath)) {
        free(filePath);
        return;
    }

    // For debugging, and avoiding including the same file twice
    BlarbVM_addFileNameToLoadedFiles(vm, filePath);

	FILE *fp = fopen(filePath, "r");
	if ( ! fp) {
		fprintf(stderr, "Failed to open '%s'\n", filePath);
		perror("fopen");
		terminateVM();
	}
    yyin = fp;
    yyfilename = filePath;

	token *line;
    while ((line = BlarbVM_scanLine(vm))) {
        // Ignore blank lines
        if (line[0].type == NEWLINE) {
            continue;
        }

        BlarbVM_addLine(vm, line);
        BlarbVM_addLineDebugInfo(vm, filePath);
    }

    yyin = stdin;
    yyfilename = NULL;

    fclose(fp);
}

void BlarbVM_addLine(BlarbVM *vm, token *line) {
	if (ISPOWEROF2(vm->lineCount) || vm->lineCount == 0) {
		// Double the size when space runs out - amortized time is O(1)
		vm->lines = vm->lineCount == 0
			? malloc(sizeof(token_t *))
			: realloc(vm->lines, sizeof(token_t *) * vm->lineCount * 2);
	}
	vm->lines[vm->lineCount++] = line;
}

