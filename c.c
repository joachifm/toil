#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aux.h"

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

struct field_list; /* forward */
struct param_list; /* forward */
struct symtab; /* forward */
struct type; /* forward */

enum {
    KLASS_CONST = 1, /* 0 is unset */
    KLASS_FUNC,
    KLASS_TYPE,
    KLASS_VAR,
    MAX_KLASS, /* for bounds checking */
};

struct param_list {
    char name[IDENT_BUF_SIZE];
    struct type* type;
    struct param_list* prev;
};

struct symtab {
    unsigned klass;
    struct symtab* prev;
    char name[IDENT_BUF_SIZE];

    /* Variant fields */
    union {
        /* when KLASS_TYPE */
        struct { struct type* spec; } type;

        /* when KLASS_CONST */
        struct { long valu; } constant;

        /* when KLASS_FUNC */
        struct {
            struct type* rettype;
            size_t arity;
            struct param_list* params;
        } func;

        /* when KLASS_VAR */
        struct {
            struct type* type;
            /*long addr;*/
        } var;
    };
};

enum {
    TYPE_BOOL = 1, /* 0 is "unset" */
    TYPE_CHAR,
    TYPE_INT,
    TYPE_ARRAY,
    TYPE_POINTER,
    TYPE_RECORD,
    MAX_TYPE, /* for bounds checking */
};

struct type {
    unsigned klass;

    /* Variant fields */
    union {
        /* when TYPE_ARRAY */
        struct {
            struct type* base;
            size_t len;
        } array;

        /* when TYPE_POINTER */
        struct {
            struct type* base;
        } pointer;

        /* when TYPE_RECORD */
        struct {
            struct type* base;
            struct field_list* fields;
        } record;
    };
};

struct field_list {
    char name[IDENT_BUF_SIZE];
    struct type* type;
    struct field_list* prev;
};

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

/* Scanner state */
static int lookch; // Input char lookahead
static int numbase = 10; // Current number base
static int looksym; // Class of last scanned token
static char ident[IDENT_MAX_LEN]; // Token value when looksym == IDENT
static size_t ident_len; // Number of chars read into ident
static long number; // Token value when LIT_INT

static struct type builtin_type_bool = (struct type){.klass = TYPE_BOOL};
static struct type builtin_type_char = (struct type){.klass = TYPE_CHAR};
static struct type builtin_type_int = (struct type){.klass = TYPE_INT};

static struct symtab* symtab_last;

static struct symtab* intern(char const* name, unsigned const klass) {
    assert(name);
    assert(klass > 0 && klass < MAX_KLASS);

    struct symtab* new = emalloc(sizeof(*new));
    *new = (struct symtab){
        .klass = klass,
        .prev = symtab_last,
    };
    strcpy(new->name, name);

    symtab_last = new;
    return new;
}

static struct symtab* resolve(char const* name, unsigned const klass) {
    assert(name);
    assert(klass > 0 && klass < MAX_KLASS);

    struct symtab* found = 0;
    for (found = symtab_last;
         found && !(found->klass == klass && STREQ(found->name, name));
         found = found->prev);

    assert(found && found->klass == klass);
    assert(found && STREQ(found->name, name));

    return found;
}

static void indent(int level) {
    while (level > 0) { putchar(' '); --level; }
}

static void describe_type(int ind, struct type const* spec) {
    assert(spec);

    indent(ind); printf("TYPE\n");
    indent(ind); printf("klass: ");
    switch (spec->klass) {
    case TYPE_BOOL:
        printf("BOOL\n");
        break;
    case TYPE_CHAR:
        printf("CHAR\n");
        break;
    case TYPE_INT:
        printf("INT\n");
        break;
    case TYPE_ARRAY:
        printf("ARRAY\n");
        indent(ind); printf("size: %zu\n", spec->array.len);
        indent(ind); printf("base:\n");
        describe_type(ind + 1, spec->array.base);
        break;
    case TYPE_POINTER:
        printf("POINTER\n");
        indent(ind); printf("base:\n");
        describe_type(ind + 1, spec->pointer.base);
        break;
    case TYPE_RECORD:
        printf("RECORD\n");
        if (spec->record.base) {
            indent(ind); printf("base:\n");
            describe_type(ind + 1, spec->record.base);
        }
        if (spec->record.fields) {
            indent(ind); printf("fields:\n");
            for (struct field_list* f = spec->record.fields; f; f = f->prev) {
                indent(ind); printf("name: %s\n", f->name);
                describe_type(ind + 1, f->type);
            }
        }
        break;
    }
}

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
        char ds[DIGIT_BUF_SIZE];
        size_t i = 0;
        do { ds[i++] = lookch; lookch = getchar(); }
        while (i < DIGIT_BUF_SIZE - 1 && isdigit(lookch));
        ds[i] = '\0';
        number = strtol(ds, 0, numbase);
        looksym = LIT_INT;
    } else if (isalpha(lookch)) {
        ident_len = 0;
        do { ident[ident_len++] = lookch; lookch = getchar(); }
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
        looksym = EQUAL;
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

static bool Size(size_t* out) {
    assert(out);

    if (looksym != LIT_INT || number < 0) {
        return false;
    }
    *out = (size_t)number;
    getsym(); // consumed number
    return true;
}

static bool TypeSpec(struct type** spec) {
    assert(spec);

    if (looksym == IDENT) { // typid
        char typid[IDENT_BUF_SIZE];
        strcpy(typid, ident);
        getsym(); // consumed ident

        if (STREQ(typid, "BOOL")) {
            *spec = &builtin_type_bool;
        } else if (STREQ(typid, "CHAR")) {
            *spec = &builtin_type_char;
        } else if (STREQ(typid, "INT")) {
            *spec = &builtin_type_int;
        } else {
            struct symtab* found = resolve(typid, KLASS_TYPE);
            if (!found) {
                fprintf(stderr, "fatal: unbound type: %s\n", typid);
                exit(1);
            }
            *spec = found->type.spec;
        }

        return true;
    } else if (looksym == ARRAY) {
        getsym(); // consumed 'ARRAY'

        // TODO factor as ConstExpr?
        size_t len = 0;
        if (looksym == IDENT) {
            char constnam[IDENT_BUF_SIZE];
            strcpy(constnam, ident);
            getsym(); // consumed ident
            struct symtab* found = resolve(constnam, KLASS_CONST);
            if (!found) {
                fprintf(stderr, "fatal: expected const\n");
                exit(1);
            }
            if (found->constant.valu < 0) {
                fprintf(stderr, "fatal: expected positive integer\n");
                exit(1);
            }
            len = (size_t)found->constant.valu;
        } else if (!Size(&len)) {
            fprintf(stderr, "fatal: expected positive integer\n");
            exit(1);
        }

        struct type* base = 0;
        if (!TypeSpec(&base)) {
            fprintf(stderr, "fatal: expected type-spec\n");
            exit(1);
        }

        struct type* newtyp = emalloc(sizeof(*newtyp));
        *newtyp = (struct type){
            .klass = TYPE_ARRAY,
            .array.len = len,
            .array.base = base,
        };
        *spec = newtyp;

        return true;
    } else if (looksym == POINTER) {
        getsym(); // consumed 'POINTER'

        struct type* base = 0;
        if (!TypeSpec(&base)) {
            fprintf(stderr, "fatal: expected type-spec\n");
            exit(1);
        }

        struct type* newtyp = emalloc(sizeof(*newtyp));
        *newtyp = (struct type){
            .klass = TYPE_POINTER,
            .pointer.base = base,
        };
        *spec = newtyp;

        return true;
    } else if (looksym == RECORD) {
        getsym(); // consumed 'RECORD'

        struct type* base = 0;
        if (looksym == LPAREN) {
            getsym(); // consumed '('
            if (!TypeSpec(&base)) {
                fprintf(stderr, "fatal: expected type-spec\n");
                exit(1);
            }
            if (looksym != RPAREN) {
                fprintf(stderr, "fatal: expected closing paren\n");
                exit(1);
            }
            getsym(); // consumed ')'
        }

        struct field_list* fields = 0;
        while (1) {
            if (looksym != IDENT) {
                break;
            }

            struct field_list* newfield = emalloc(sizeof(*newfield));
            *newfield = (struct field_list){.prev = fields};
            strcpy(newfield->name, ident);
            getsym(); // consumed ident
            if (!TypeSpec(&newfield->type)) {
                fprintf(stderr, "fatal: expected type-spec\n");
                exit(1);
            }
            fields = newfield;
        }

        struct type* newtyp = emalloc(sizeof(*newtyp));
        *newtyp = (struct type){
            .klass = TYPE_RECORD,
            .record.base = base,
            .record.fields = fields,
        };
        *spec = newtyp;

        return true;
    } else {
        return false;
    }
}

static bool TypeDecl(void) {
    if (looksym != TYPE) {
        return false;
    }
    getsym(); // consumed 'TYPE'

    if (looksym != IDENT) {
        fprintf(stderr, "fatal: expected type name\n");
        return false;
    }

    char typnam[IDENT_BUF_SIZE];
    strcpy(typnam, ident);
    getsym(); // consumed ident

    struct type* typspec = 0;
    if (!TypeSpec(&typspec)) {
        fprintf(stderr, "fatal: expected type-spec\n");
        exit(1);
    }

    struct symtab* binds = intern(typnam, KLASS_TYPE);
    binds->type.spec = typspec;

    describe_type(1, binds->type.spec);

    return true;
}

static bool VarDecl(void) {
    if (looksym != VAR) {
        return false;
    }
    getsym(); // consumed 'VAR'

    if (looksym != IDENT) {
        fprintf(stderr, "fatal: expected identifier\n");
        exit(1);
    }
    char varnam[IDENT_BUF_SIZE];
    strcpy(varnam, ident);
    getsym(); // consumed ident

    struct type* thetype = 0;
    if (!TypeSpec(&thetype)) {
        fprintf(stderr, "fatal: expected type-spec\n");
        exit(1);
    }

    struct symtab* binds = intern(varnam, KLASS_VAR);
    binds->var.type = thetype;

    return true;
}

static bool ConstDecl(void) {
    if (looksym != CONST) {
        return false;
    }
    getsym(); // consumed 'CONST'

    if (looksym != IDENT) {
        fprintf(stderr, "fatal: expected ident\n");
        exit(1);
    }
    char constnam[IDENT_BUF_SIZE];
    strcpy(constnam, ident);
    getsym(); // consumed ident

    if (looksym != LIT_INT) {
        fprintf(stderr, "fatal: expected const-expr\n");
        exit(1);
    }
    long expr = number;
    getsym(); // consumed const-expr

    struct symtab* binds = intern(constnam, KLASS_CONST);
    binds->constant.valu = expr;

    return true;
}

static bool FuncDecl(void) {
    if (looksym != FUNC) {
        return false;
    }
    getsym(); // consumed 'FUNC'

    if (looksym != IDENT) {
        fprintf(stderr, "fatal: expected ident\n");
        exit(1);
    }
    char funcnam[IDENT_BUF_SIZE];
    strcpy(funcnam, ident);
    getsym(); // consumed ident

    size_t arity = 0;
    struct param_list* params = 0;

    if (looksym == LPAREN) {
        getsym(); // consumed '('
        do {
            if (looksym != IDENT) {
                fprintf(stderr, "fatal: expected param identifier\n");
                exit(1);
            }
            char paramnam[IDENT_BUF_SIZE];
            strcpy(paramnam, ident);
            getsym(); // consumed ident

            struct type* type = 0;
            if (!TypeSpec(&type)) {
                fprintf(stderr, "fatal: expected type-spec\n");
                exit(1);
            }

            struct param_list* newparam = emalloc(sizeof(*newparam));
            *newparam = (struct param_list){};
            strcpy(newparam->name, paramnam);
            newparam->type = type;

            ++arity;
        } while (looksym != RPAREN);
        getsym(); // consumed ')'
    }

    struct type* rettype = 0;
    if (!TypeSpec(&rettype)) {
        fprintf(stderr, "fatal: expected type-spec\n");
        exit(1);
    }

    struct symtab* newfunc = intern(funcnam, KLASS_FUNC);
    newfunc->func.arity = arity;
    newfunc->func.params = params;
    newfunc->func.rettype = rettype;

    // TODO body

    if (looksym != END) {
        fprintf(stderr, "fatal: expected 'END'\n");
        exit(1);
    }
    getsym(); // consumed 'END'

    return true;
}

static bool Decl(void) {
    return ConstDecl() || FuncDecl() || VarDecl() || TypeDecl();
}

int main(int argc, char** argv) {
    lookch = getchar(); getsym();

    while (lookch != EOF) {
        Decl();
    }

    return 0;
}
