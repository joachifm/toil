#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STREQ(S1, S2) ((_Bool)(strcmp(S1, S2) == 0))

/* The maximum number of digits in a number.  A buffer holding a number
 * literal must be at least DIGIT_MAX+1 to account for the NUL.
 */
#define DIGIT_MAX 4

/* The maximum number of characters in an identifier, including the
 * trailing NUL.
 */
#define IDENT_MAX_LEN 32

/* Token classes */
enum {
    IDENT = 1, /* 0 is none */
    LITERAL_INT,

    MODULE,

    ASSIGN,

    IF,
    THEN,
    ELIF,
    ELSE,

    EQUAL,
    NOT,
    AND,
    OR,

    COMMA,
    PERIOD,
    COLON,
    SEMICOLON,

    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACK,
    RBRACK,
};

/* Scanner state */
static int lookch; // Input char lookahead
static int numbase = 10; // Current number base
static int looksym; // Class of last scanned token
static char ident[IDENT_MAX_LEN]; // Token value when looksym == IDENT
static size_t ident_len; // Number of chars read into ident
static long number; // Token value when LITERAL_INT

static int accept_char(int ch) {
    if (lookch == ch) {
        lookch = getchar();
        return 1;
    } else {
        return 0;
    }
}

static int getsym() {
    looksym = 0;

    while (isspace(lookch)) lookch = getchar();

    if (lookch == EOF) return EOF;

    if (isdigit(lookch)) {
        char ds[DIGIT_MAX + 1];
        size_t i = 0;
        do { ds[i++] = lookch; lookch = getchar(); }
        while (i < DIGIT_MAX && isdigit(lookch));
        ds[i] = '\0';
        number = strtol(ds, 0, numbase);
        looksym = LITERAL_INT;
    } else if (lookch == '_' || isalpha(lookch)) {
        ident_len = 0;
        do { ident[ident_len++] = lookch; lookch = getchar(); }
        while (ident_len < IDENT_MAX_LEN - 1 && isalnum(lookch));
        ident[ident_len] = '\0';
        looksym =
            STREQ(ident, "MODULE") ? MODULE :
            STREQ(ident, "IF") ? IF :
            STREQ(ident, "THEN") ? THEN :
            STREQ(ident, "ELIF") ? ELIF :
            STREQ(ident, "ELSE") ? ELSE :
            IDENT;
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
        looksym = EQUAL;
    } else if (accept_char(':')) {
        if (accept_char('=')) {
            looksym = ASSIGN;
        } else {
            looksym = COLON;
        }
    } else {
        printf("Unexpected character '%c'\n", lookch);
        lookch = getchar();
        looksym = 0;
    }

    return looksym;
}

int main(int argc, char** argv) {
    lookch = getchar();
    while (getsym() != EOF) {
        switch (looksym) {
        case IDENT:
            printf("IDENT %s\n", ident);
            break;
        case LITERAL_INT:
            printf("NUMBER %ld\n", number);
            break;
        case LPAREN:
            puts("LPAREN");
            break;
        case RPAREN:
            puts("RPAREN");
            break;
        case COMMA:
            puts("COMMA");
            break;
        case PERIOD:
            puts("PERIOD");
            break;
        case COLON:
            puts("COLON");
            break;
        case SEMICOLON:
            puts("SEMICOLON");
            break;
        case EQUAL:
            puts("EQUAL");
            break;
        case ASSIGN:
            puts("ASSIGN");
            break;
        }
    }
}
