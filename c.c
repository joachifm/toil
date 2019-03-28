#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "aux.h"
#include "lex.h"

struct symtab; /* forward */
struct type; /* forward */
struct binds;

enum {
    KLASS_CONST = 1, /* 0 is unset */
    KLASS_FUNC,
    KLASS_TYPE,
    KLASS_VAR,
    MAX_KLASS, /* for bounds checking */
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

struct binds {
    char name[IDENT_BUF_SIZE];
    struct type* type;
    struct binds* prev;
};

struct type {
    unsigned kind;

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
            struct binds* fields;
        } record;
    };
};

struct symtab {
    struct symtab* prev;
    char name[IDENT_BUF_SIZE];

    /* Variant fields */
    unsigned klass;

    union {
        /* when KLASS_TYPE */
        struct { struct type* spec; } type;

        /* when KLASS_CONST */
        struct { long valu; } constant;

        /* when KLASS_FUNC */
        struct {
            struct type* rettype;
            size_t arity;
            struct binds* params;
        } func;

        /* when KLASS_VAR */
        struct {
            struct type* type;
            /*long addr;*/
        } var;
    };
};

static struct type builtin_type_bool = (struct type){.kind = TYPE_BOOL};
static struct type builtin_type_char = (struct type){.kind = TYPE_CHAR};
static struct type builtin_type_int = (struct type){.kind = TYPE_INT};

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
    switch (spec->kind) {
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
            for (struct binds* f = spec->record.fields; f; f = f->prev) {
                indent(ind); printf("name: %s\n", f->name);
                describe_type(ind + 1, f->type);
            }
        }
        break;
    }
}

static bool ConstExpr(long* out) {
    assert(out);

    if (looksym == LIT_INT) {
        *out = numval;
        lex_getsym(); // consumed number
        return true;
    } else if (looksym == IDENT) {
        struct symtab* found = resolve(ident, KLASS_CONST);
        if (found) {
            lex_getsym(); // consumed ident
            *out = found->constant.valu;
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

static bool TypeSpec(struct type** spec) {
    assert(spec);

    if (looksym == IDENT) { // typid
        char typid[IDENT_BUF_SIZE];
        strcpy(typid, ident);
        lex_getsym(); // consumed ident

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
        lex_getsym(); // consumed 'ARRAY'

        long lenval = 0;
        if (!ConstExpr(&lenval)) {
            fprintf(stderr, "fatal: expected const-expr\n");
            exit(1);
        }
        if (lenval < 0) {
            fprintf(stderr, "fatal: expected positive integer\n");
            exit(1);
        }

        size_t len = (size_t)lenval;

        struct type* base = 0;
        if (!TypeSpec(&base)) {
            fprintf(stderr, "fatal: expected type-spec\n");
            exit(1);
        }

        struct type* newtyp = emalloc(sizeof(*newtyp));
        *newtyp = (struct type){
            .kind = TYPE_ARRAY,
            .array.len = len,
            .array.base = base,
        };
        *spec = newtyp;

        return true;
    } else if (looksym == POINTER) {
        lex_getsym(); // consumed 'POINTER'

        struct type* base = 0;
        if (!TypeSpec(&base)) {
            fprintf(stderr, "fatal: expected type-spec\n");
            exit(1);
        }

        struct type* newtyp = emalloc(sizeof(*newtyp));
        *newtyp = (struct type){
            .kind = TYPE_POINTER,
            .pointer.base = base,
        };
        *spec = newtyp;

        return true;
    } else if (looksym == RECORD) {
        lex_getsym(); // consumed 'RECORD'

        struct type* base = 0;
        if (looksym == LPAREN) {
            lex_getsym(); // consumed '('
            if (!TypeSpec(&base)) {
                fprintf(stderr, "fatal: expected type-spec\n");
                exit(1);
            }
            if (looksym != RPAREN) {
                fprintf(stderr, "fatal: expected closing paren\n");
                exit(1);
            }
            lex_getsym(); // consumed ')'
        }

        struct binds* fields = 0;
        while (1) {
            if (looksym != IDENT) {
                break;
            }
            char fieldnam[IDENT_BUF_SIZE];
            strcpy(fieldnam, ident);
            lex_getsym(); // consumed ident

            struct binds* newfield = emalloc(sizeof(*newfield));
            *newfield = (struct binds){.prev = fields};
            strcpy(newfield->name, fieldnam);
            if (!TypeSpec(&newfield->type)) {
                fprintf(stderr, "fatal: expected type-spec\n");
                exit(1);
            }
            fields = newfield;
        }

        struct type* newtyp = emalloc(sizeof(*newtyp));
        *newtyp = (struct type){
            .kind = TYPE_RECORD,
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
    lex_getsym(); // consumed 'TYPE'

    if (looksym != IDENT) {
        fprintf(stderr, "fatal: expected type name\n");
        return false;
    }

    char typnam[IDENT_BUF_SIZE];
    strcpy(typnam, ident);
    lex_getsym(); // consumed ident

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
    lex_getsym(); // consumed 'VAR'

    if (looksym != IDENT) {
        fprintf(stderr, "fatal: expected identifier\n");
        exit(1);
    }
    char varnam[IDENT_BUF_SIZE];
    strcpy(varnam, ident);
    lex_getsym(); // consumed ident

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
    lex_getsym(); // consumed 'CONST'

    if (looksym != IDENT) {
        fprintf(stderr, "fatal: expected ident\n");
        exit(1);
    }
    char constnam[IDENT_BUF_SIZE];
    strcpy(constnam, ident);
    lex_getsym(); // consumed ident

    long expr = 0;
    if (!ConstExpr(&expr)) {
        fprintf(stderr, "fatal: expected const-expr\n");
        exit(1);
    }

    struct symtab* binds = intern(constnam, KLASS_CONST);
    binds->constant.valu = expr;

    return true;
}

static bool FuncDecl(void) {
    if (looksym != FUNC) {
        return false;
    }
    lex_getsym(); // consumed 'FUNC'

    if (looksym != IDENT) {
        fprintf(stderr, "fatal: expected ident\n");
        exit(1);
    }
    char funcnam[IDENT_BUF_SIZE];
    strcpy(funcnam, ident);
    lex_getsym(); // consumed ident

    size_t arity = 0;
    struct binds* params = 0;

    if (looksym == LPAREN) {
        lex_getsym(); // consumed '('
        do {
            if (looksym != IDENT) {
                fprintf(stderr, "fatal: expected param identifier\n");
                exit(1);
            }
            char paramnam[IDENT_BUF_SIZE];
            strcpy(paramnam, ident);
            lex_getsym(); // consumed ident

            struct type* type = 0;
            if (!TypeSpec(&type)) {
                fprintf(stderr, "fatal: expected type-spec\n");
                exit(1);
            }

            struct binds* newparam = emalloc(sizeof(*newparam));
            *newparam = (struct binds){};
            strcpy(newparam->name, paramnam);
            newparam->type = type;

            ++arity;
        } while (looksym != RPAREN);
        lex_getsym(); // consumed ')'
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
    lex_getsym(); // consumed 'END'

    return true;
}

static bool Decl(void) {
    return ConstDecl() || FuncDecl() || VarDecl() || TypeDecl();
}

int main(int argc, char** argv) {
    lex_init();
    while (Decl());
    return 0;
}
