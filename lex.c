#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "aux.h"
#include "lex.h"

int looksym = 0;
int numbase = 10;
long numval = 0;
char ident[IDENT_MAX_LEN];
size_t ident_len = 0;

static int lookch; /* Input char lookahead */

static int accept_char(int ch) {
    if (lookch == ch) {
        lookch = getchar();
        return 1;
    } else {
        return 0;
    }
}

int lex_getsym() {
    looksym = 0;

    while (isspace(lookch)) lookch = getchar();

    if (lookch == EOF) return EOF;

    if (isdigit(lookch)) {
        char ds[DIGIT_BUF_SIZE];
        size_t i = 0;
        do { ds[i++] = (char)lookch; lookch = getchar(); }
        while (i < DIGIT_BUF_SIZE - 1 && isdigit(lookch));
        ds[i] = '\0';
        numval = strtol(ds, 0, numbase);
        looksym = LIT_INT;
    } else if (isalpha(lookch)) {
        ident_len = 0;
        do { ident[ident_len++] = (char)lookch; lookch = getchar(); }
        while (ident_len < IDENT_BUF_SIZE - 1 && (isalnum(lookch) || lookch == '_'));
        ident[ident_len] = '\0';
        looksym =
            STREQ(ident, "MODULE") ? MODULE :

            STREQ(ident, "IF") ? IF :
            STREQ(ident, "THEN") ? THEN :
            STREQ(ident, "ELIF") ? ELIF :
            STREQ(ident, "ELSE") ? ELSE :

            STREQ(ident, "CONST") ? CONST :
            STREQ(ident, "FUNC") ? FUNC :
            STREQ(ident, "PROC") ? PROC :
            STREQ(ident, "TYPE") ? TYPE :
            STREQ(ident, "VAR") ? VAR :

            STREQ(ident, "REPEAT") ? REPEAT :
            STREQ(ident, "UNTIL") ? UNTIL :
            STREQ(ident, "WHILE") ? WHILE :
            STREQ(ident, "DO") ? DO :

            STREQ(ident, "BEGIN") ? BEGIN :
            STREQ(ident, "END") ? END :

            STREQ(ident, "ARRAY") ? ARRAY :
            STREQ(ident, "POINTER") ? POINTER :
            STREQ(ident, "RECORD") ? RECORD :

            IDENT;
    } else if (accept_char('_')) {
        looksym = UNDERSCORE;
    } else if (accept_char('(')) {
        // TODO fails to handle nested comments (* (* foo *) *)
        if (accept_char('*')) { // comment begin
            do {
                if (accept_char('*') && accept_char(')')) {
                    break;
                }
            } while ((lookch = getchar()) != EOF);
            looksym = 0;
        } else {
            looksym = LPAREN;
        }
    } else if (accept_char(')')) {
        looksym = RPAREN;
    } else if (accept_char(',')) {
        looksym = COMMA;
    } else if (accept_char('.')) {
        looksym = PERIOD;
    } else if (accept_char('=')) {
        looksym = EQ;
    } else if (accept_char('>')) {
        if (accept_char('=')) {
            looksym = GTE;
        } else {
            looksym = GT;
        }
    } else if (accept_char('<')) {
        if (accept_char('=')) {
            looksym = LTE;
        } else {
            looksym = LT;
        }
    } else if (accept_char(':')) {
        if (accept_char('=')) {
            looksym = ASSIGN;
        } else {
            looksym = COLON;
        }
    } else if (accept_char('+')) {
        looksym = PLUS;
    } else if (accept_char('-')) {
        looksym = MINUS;
    } else if (accept_char('*')) {
        looksym = STAR;
    } else if (accept_char('{')) {
        looksym = LBRACE;
    } else if (accept_char('}')) {
        looksym = RBRACE;
    } else if (accept_char('[')) {
        looksym = LBRACK;
    } else if (accept_char(']')) {
        looksym = RBRACK;
    } else if (accept_char('\'')) {
        looksym = QUOTE;
    } else if (accept_char('|')) {
        looksym = BAR;
    } else {
        printf("Unexpected character '%c'\n", lookch);
        lookch = getchar();
        looksym = 0;
    }

    return looksym;
}

void lex_init(void) {
    lookch = getchar();
    lex_getsym();
}
