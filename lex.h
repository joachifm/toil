#pragma once

/* The maximum number of digits in a number.  Buffers
 * holding digits must hold at least DIGIT_BUF_SIZE elements.
 */
#define DIGIT_MAX 4

/* The maximum number of characters in an identifier. */
#define IDENT_MAX_LEN ((size_t)31)

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

    EQ,
    LT,
    GT,
    LTE,
    GTE,

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

    BEGIN,
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

    MAX_TOKEN, /* for bounds checking */
};

/* Initialize the lexer state. */
void lex_init(void);

/* Get the next token from the input stream. */
int lex_getsym(void);

extern int looksym; /* Class of last scanned token */
extern int numbase; /* Current number base */
extern long numval; /* Value of LIT_INT */
extern char ident[IDENT_MAX_LEN]; /* Value of IDENT */
extern size_t ident_len; /* Number of chars read into ident */
