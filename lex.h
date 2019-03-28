#pragma once

/* The maximum number of digits in a number.  Buffers
 * holding digits must hold at least DIGIT_BUF_SIZE elements.
 */
#define DIGIT_MAX 4

/* The maximum number of characters in an identifier. */
#define IDENT_MAX_LEN 31

/* Size of buffer holding digits, including trailing NUL. */
#define DIGIT_BUF_SIZE (DIGIT_MAX + 1)

/* Size of buffer holding identifier, including trailing NUL. */
#define IDENT_BUF_SIZE (IDENT_MAX_LEN + 1)

/* Token classes */
enum {
    IDENT = 1, /* 0 is none */

    LIT_CHAR,
    LIT_INT,

    CONST,
    FUNC,
    PROC,
    TYPE,
    VAR,

    ARRAY,
    RECORD,
    POINTER,

    EQUAL,

    AND,
    NOT,
    OR,

    ASSIGN,

    MINUS,
    PLUS,
    STAR,

    IF,
    THEN,
    ELIF,
    ELSE,

    REPEAT,
    UNTIL,
    WHILE,
    DO,
    END,

    MODULE,

    BAR,
    COLON,
    COMMA,
    PERIOD,
    QUOTE,
    SEMICOLON,
    UNDERSCORE,

    LBRACE,
    RBRACE,

    LBRACK,
    RBRACK,

    LPAREN,
    RPAREN,
};

void lex_init(void);
int lex_getsym(void);

extern int looksym; /* Class of last scanned token */
extern int numbase; /* Current number base */
extern long numval; /* Token value when LIT_INT */
extern char ident[IDENT_MAX_LEN]; /* Token value when IDENT */
extern size_t ident_len; /* Number of chars read into ident */
