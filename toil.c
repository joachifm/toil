#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>

#define STREQ(S1, S2) (strcmp(S1, S2) == 0)

noreturn void Error(const char* fmt, ...) {
    int saved_errno = errno;

    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "fatal: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    int ecode = EXIT_FAILURE;
    if (saved_errno) {
        ecode = -saved_errno;
        fprintf(stderr, ": %s", strerror(saved_errno));
    }
    fprintf(stderr, "\n");
    exit(ecode);
}

/*
 * Labels
 */

char const* NewLabel(void) {
#define LABEL_NUM_MAX 1000
#define LABEL_BUF_SIZ (1 + 3 + 1L) /* 'L' + N digits + NUL */
    static unsigned next = 0;
    static char ss[LABEL_NUM_MAX][LABEL_BUF_SIZ] = {{0}};

    if (next > LABEL_NUM_MAX - 1) Error("Exhausted label storage");
    unsigned this = next++;
    char* s = ss[this];
    snprintf(s, LABEL_BUF_SIZ, "L%u", this);
    return s;
#undef LABEL_NUM_MAX
#undef LABEL_BUF_SIZ
}

void PutLabel(char const* lbl) {
    printf("%s:\n", lbl);
}

/*
 * Scanning
 */

#define TOKEN_BUF_SIZ 16L

static int look;
static char tok[TOKEN_BUF_SIZ];
static int sym;

static char GetChar(void);

#define SkipSpaces() while (isspace(look)) { GetChar(); }

/* Advance the character lookahead */
char GetChar(void) {
    if (look == EOF) Error("Unexpected end of input");
    char saved = (char)look;
    look = getchar();
    return saved;
}

enum {
    IF = 'A', /* reserve 'A'...'Z' for keywords */
    THEN,
    ELSE,
    ENDIF,
    WHILE,
    ENDWHILE,
};

static void Keyword(void) {
    if (!(sym == 'i' && isupper(tok[0]))) return;
    sym = STREQ(tok, "IF") ? IF
        : STREQ(tok, "THEN") ? THEN
        : STREQ(tok, "ELSE") ? ELSE
        : STREQ(tok, "ENDIF") ? ENDIF
        : STREQ(tok, "WHILE") ? WHILE
        : STREQ(tok, "ENDWHILE") ? ENDWHILE
        : ({ Error("Unrecognized keyword: %s", tok); 0; });
}

/* Get the next token from the input stream. */
static int GetSym(void) {
    SkipSpaces();

    if (isalpha(look)) {
        sym = 'i';
        size_t len = 0;
        do { tok[len++] = GetChar(); }
        while (len < TOKEN_BUF_SIZ - 1 && (isalpha(look) || look == '_'));
        tok[len] = '\0';
    } else if (isdigit(look)) {
        sym = '#';
        size_t len = 0;
        do { tok[len++] = GetChar(); }
        while (len < TOKEN_BUF_SIZ - 1 && isdigit(look));
        tok[len] = '\0';
    } else if (look == EOF) {
        sym = EOF;
        tok[0] = '\0';
    } else {
        sym = look;
        tok[0] = '\0';
        GetChar();
    }
    return sym;
}

/*
 * Parsing and translation
 */

static void Expression(void);

static void Factor(void) {
    puts("<factor>");

    if (sym == '(') {
        GetSym(); /* consume '(' */
        Expression();
        if (look != ')') Error("Expected ')'");
    } else if (sym == 'i') {
        printf("IDENT %s\n", tok);
        GetSym(); /* consume ident */
    } else if (sym == '#') {
        printf("LITERAL %s\n", tok);
        GetSym(); /* consume literal */
    } else {
        Error("Expected factor");
    }
}

static void Term(void) {
    puts("<term>");

    Factor();
    while (sym == '*' || sym == '/') {
        /* TODO Push operand */
        if (sym == '*') {
            GetSym(); /* consume oper */
            Factor();
            puts("MUL");
        } else if (sym == '/') {
            GetSym(); /* consume oper */
            Factor();
            puts("DIV");
        }
    }
}

void Expression(void) {
    puts("<expression>");

    Term();
    while (sym == '+' || sym == '-') {
        /* TODO Push operand */

        if (sym == '+') {
            GetSym(); /* consume oper */
            Term();
            puts("ADD");
        } else if (sym == '-') {
            GetSym(); /* consume oper */
            Term();
            puts("SUB");
        }
    }
}

static void Relation(void) {
    Expression();
    if (sym == '>' || sym == '<' || sym == '=') {
        /* TODO Push operand */

        if (sym == '>') {
            GetSym(); /* consume oper */
            if (sym == '=') {
                GetSym(); /* consume oper */
                puts("<greater-than-or-equal>");
            } else {
                puts("<greater>");
            }
        } else if (sym == '<') {
            GetSym(); /* consume oper */
            if (sym == '=') {
                GetSym(); /* consume oper */
                puts("<less-than-or-equal>");
            } else {
                puts("<less-than>");
            }
        } else if (sym == '=') {
            GetSym(); /* consume oper */
            puts("<equals>");
        }
    }
}

static void Block(void);

static void Assignment(void) {
    puts("<assignment>");

    if (sym != 'i') Error("Expected identifier");
    char name[TOKEN_BUF_SIZ];
    strcpy(name, tok);
    GetSym(); /* consumed ident */

    if (sym != '=') Error("Expected '='");
    GetSym(); /* consumed '=' */

    Expression();
    /* TODO Generate code to store */
}

static void While(void) {
    if (sym != WHILE) Error("Expected WHILE");
    GetSym(); /* consumed 'WHILE' */

    char const* l1 = NewLabel();
    char const* l2 = NewLabel();

    PutLabel(l1);
    Expression();
    printf("JMPZ %s\n", l2); /* bail */
    Block();

    if (sym != ENDWHILE) Error("Expected ENDWHILE");
    GetSym(); /* consumed 'ENDWHILE' */

    printf("JMP %s\n", l1); /* recur */
    PutLabel(l2);
}

static void IfThenElse(void) {
    puts("<if-then-else>");

    if (sym != IF) Error("Expected IF");
    GetSym(); /* consumed 'IF' */

    char const* l1 = NewLabel();
    char const* l2 = NewLabel();

    Expression();

    Keyword();
    if (sym != THEN) Error("Expected THEN");
    GetSym(); /* consumed 'THEN' */

    printf("JMPZ %s\n", l1); /* jump to else */
    Block();
    printf("JMP %s\n", l2);  /* step over else */

    if (sym != ELSE) Error("Expected ELSE");
    GetSym(); /* consumed 'ELSE' */

    PutLabel(l1);
    Block();

    PutLabel(l2);

    if (sym != ENDIF) Error("Expected ENDIF");
    GetSym(); /* consumed 'ENDIF' */
}

void Statement(void) {
    puts("<stmt>");
    if      (sym == IF)    IfThenElse();
    else if (sym == WHILE) While();
    else if (sym == 'i')   Assignment();
    else                   return;
}

void Block(void) {
    puts("<block>");
    Keyword();
    /* Continue until no more input or we reach a "end of block" keyword */
    while (look != EOF
           && sym != ELSE     /* ends THEN block */
           && sym != ENDIF    /* ends ELSE block */
           && sym != ENDWHILE /* ends WHILE block */
        ) {
        Statement();
        Keyword();
    }
}

/*
 * Main entry
 */

static void Init(void) {
    look = getchar();
    GetSym();
}

int main(void) {
    Init();
    Block();
}
