%{
    /* BlarbVM Lexer */
    #include "../src/vm.h"
    char *yyfilename; // Set in scanner.c
%}

%option yylineno
%option noyywrap

%{ 
    /* REGEX Variable Initialization */ 
%}
ws            [ ]|[\t]
integers      -?[0-9]+
str           \"(\\.|[^\"])*\"
chr           \'(\\.|[^\'])\'
identifiers   ([a-zA-Z_])([a-zA-Z0-9_])*
label         "#"{identifiers}
label_call    {identifiers}
newline       [\n]|[\r\n]|[\r]
eol           $
comment       ;.*$

%%

{integers}      return INTEGER;
{label_call}    return LABEL_CALL;
{str}           return STR;
{chr}           return CHR;
{label}         return LABEL;
{newline}       return NEWLINE;
"@"             return INCLUDE;
"~"             return REG_STORE;
"$"             return REG_GET;
"^"             return STACK_POP;
"!"             return NAND;
"?"             return CONDITION;
"%"             return SYS_CALL;
"="             return HEAP_SWAP;

{comment} {}
{ws} {}
. {fprintf(stderr, "Invalid token: '%s' on line %d in %s\n",
           yytext, yylineno, yyfilename); exit(1);}
