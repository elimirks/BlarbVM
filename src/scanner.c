#include "main.h"
#include "vm.h"
#include "scanner.h"
#include "../obj/blarb.yy.c"

// Scans a line of tokens using yylex
token* BlarbVM_scanLine(BlarbVM *vm) {
    int tokenCount = 0;
    // 1024 tokens should be way more than enough.
    token *line = malloc(sizeof(token) * 1024);
    token_t tokenType;

    while ((tokenType = yylex())) {
        // In general, we just care about the type.
        line[tokenCount++].type = tokenType;

        if (tokenType == INTEGER) {
            line[tokenCount - 1].val = strtoull(yytext, 0, 10);
        } else if (tokenType == FUNCTION_CALL) {
            int len = strlen(yytext);
            line[tokenCount - 1].str = malloc(len + 1);
            line[tokenCount - 1].str[len] = '\0';
            strncpy(line[tokenCount - 1].str, yytext, len);
        } else if (tokenType == LABEL) {
            char *text = (char*)yytext + 1; // Exclude the hash
            int len = strlen(text);
            line[tokenCount - 1].str = malloc(len + 1);
            line[tokenCount - 1].str[len] = '\0';
            strncpy(line[tokenCount - 1].str, text, len);

            BlarbVM_addLabelPointer(vm, text, vm->lineCount);
        } else if (tokenType == STR) {
            char *text = (char*)yytext + 1; // Exclude quotes
            int len = strlen(yytext) - 1; // ^^
            line[tokenCount - 1].str = malloc(len + 1);
            line[tokenCount - 1].str[len] = '\0';
            strncpy(line[tokenCount - 1].str, yytext, len);
        } else if (tokenType == NEWLINE) {
            break;
        }
    }

    // If this is the EOF (no more tokens)
    if (tokenCount == 0) {
        free(line);
        return 0;
    }

    // Resize the memory chunk so we don't waste memory on small lines
    return realloc(line, sizeof(token) * tokenCount);
}

void BlarbVM_loadFile(BlarbVM *vm, char *fileName) {
	FILE *fp = fopen(fileName, "r");
	if ( ! fp) {
		fprintf(stderr, "Failed to open '%s'\n", fileName);
		perror("fopen");
		terminateVM();
	}
    yyin = fp;

	token *line;
    while ((line = BlarbVM_scanLine(vm))) {
        BlarbVM_addLine(vm, line);
    }

    yyin = stdin;
	fclose(fp);
}

void BlarbVM_addLabelPointer(BlarbVM *vm, char *name, int line) {
	if (vm->labelPointerCount % 2 == 0) {
		// Double the size when space runs out - amortized time is O(1)
		vm->labelPointers = vm->labelPointerCount == 0
			? malloc(sizeof(LabelPointer) * 2)
			: realloc(vm->labelPointers, sizeof(LabelPointer) * vm->labelPointerCount * 2);
	}

	LabelPointer *newLabel = &vm->labelPointers[vm->labelPointerCount];
	newLabel->name = malloc(sizeof(char) * strlen(name));
	strcpy(newLabel->name, name);
	newLabel->line = line;

	vm->labelPointerCount++;
}

void BlarbVM_addLine(BlarbVM *vm, token *line) {
	if (vm->lineCount % 2 == 0) {
		// Double the size when space runs out - amortized time is O(1)
		vm->lines = vm->lineCount == 0
			? malloc(sizeof(token_t *) * 1)
			: realloc(vm->lines, sizeof(token_t *) * vm->lineCount * 2);
	}
	vm->lines[vm->lineCount++] = line;
}

