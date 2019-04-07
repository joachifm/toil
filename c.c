#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "aux.h"
#include "lex.h"

struct binds; /* forward */
struct item; /* forward */
struct symtab; /* forward */
struct type; /* forward */

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

struct scope; /* forward */

struct scope {
    struct scope* parent;
    struct symtab* env;
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

static struct scope* scope;

static void enter_scope(void) {
    struct scope* new_scope = emalloc(sizeof(*new_scope));
    *new_scope = (struct scope){.parent = scope};

    struct symtab* new_env = emalloc(sizeof(*new_env));
    *new_env = (struct symtab){};
    new_scope->env = new_env;

    scope = new_scope;
}

static void leave_scope(void) {
    puts("leave scope");
    struct scope* parent = scope->parent;

    struct symtab* env = scope->env;
    /* TODO leaks memory */
    while (env) {
        struct symtab* prev = env->prev; /* save link */
        *env = (struct symtab){}; /* clear memory */
        free(env); /* release storage */
        env = prev; /* next */
    }

    *scope = (struct scope){};
    free(scope);

    scope = parent;
}

static size_t type_size_of(struct type const* spec) {
    assert(spec);

    switch (spec->kind) {
    case TYPE_BOOL:
        return 1;
    case TYPE_CHAR:
        return 1;
    case TYPE_INT:
        return sizeof(int);
    case TYPE_ARRAY:
        return type_size_of(spec->array.base) * spec->array.len;
    case TYPE_POINTER:
        return sizeof(void*);
    case TYPE_RECORD: {
        size_t total = spec->record.base ? type_size_of(spec->record.base) : 0;
        for (struct binds* fld = spec->record.fields; fld; fld = fld->prev) {
            total += type_size_of(fld->type);
        }
        return total;
    }
    }

    return 0;
}

static struct symtab* intern(char const* name, unsigned const klass) {
    assert(name);
    assert(klass > 0 && klass < MAX_KLASS);

    struct symtab* new = emalloc(sizeof(*new));
    *new = (struct symtab){
        .klass = klass,
        .prev = scope ? scope->env : 0,
    };
    strcpy(new->name, name);

    scope->env = new;
    return new;
}

static struct symtab* resolve(char const* name, unsigned const klass) {
    assert(name);
    assert(klass > 0 && klass < MAX_KLASS);

    struct symtab* found = 0;
    for (found = scope->env;
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

/* Accept a nullary terminal symbol.  The lookahead is advanced iff it matches.
 *
 * @param  sym the symbol to match
 * @return true if matched, false otherwise
 */
static bool Accept(int sym) {
    assert(sym > 0 && sym < MAX_TOKEN);
    if (looksym != sym) {
        return false;
    }
    lex_getsym();
    return true;
}

/* A wrapper for Accept() that aborts the program if lookahead fails
 * to match.
 */
static void Expect(int sym) {
    if (!Accept(sym))
        fatal("expected %d\n", sym);
}

/* Accept any identifier.  If current symbol is an identifier, the identifier
 * name is copied into the supplied buffer and lookahead is advanced.
 */
static bool Identifier(char* out) {
    assert(out);
    if (looksym != IDENT) {
        return false;
    }
    strncpy(out, ident, IDENT_MAX_LEN);
    lex_getsym();
    return true;
}

/* Accept any number.  If current symbol is a number, it is copied
 * to the supplied buffer and lookahead is advanced.
 */
static bool Number(long* out) {
    assert(out);
    if (looksym != LIT_INT) {
        return false;
    }
    *out = numval;
    lex_getsym();
    return true;
}

static bool ConstVar(long* out) {
    assert(out);

    char varnam[IDENT_BUF_SIZE] = {0};
    if (!Identifier(varnam))
        return false;

    struct symtab* found = resolve(varnam, KLASS_CONST);
    if (!found)
        fatal("unbound identifier: %s\n", varnam);
    *out = found->constant.valu;
    return true;
}

static bool ConstExpr(long* out) {
    assert(out);
    return Number(out) || ConstVar(out);
}

static bool TypeSpec(struct type** spec) {
    assert(spec);

    char typid[IDENT_BUF_SIZE];
    if (Identifier(typid)) {
        if (STREQ(typid, "BOOL")) {
            *spec = &builtin_type_bool;
        } else if (STREQ(typid, "CHAR")) {
            *spec = &builtin_type_char;
        } else if (STREQ(typid, "INT")) {
            *spec = &builtin_type_int;
        } else {
            struct symtab* found = resolve(typid, KLASS_TYPE);
            if (!found)
                fatal("unbound type: %s\n", typid);
            *spec = found->type.spec;
        }
        return true;
    } else if (Accept(ARRAY)) {
        long lenval = 0;
        if (!ConstExpr(&lenval))
            fatal("expected const-expr\n");
        if (lenval < 0)
            fatal("expected positive integer\n");

        struct type* base = 0;
        if (!TypeSpec(&base))
            fatal("expected type-spec\n");

        struct type* newtyp = emalloc(sizeof(*newtyp));
        *newtyp = (struct type){
            .kind = TYPE_ARRAY,
            .array.len = (size_t)lenval,
            .array.base = base,
        };
        *spec = newtyp;

        return true;
    } else if (Accept(POINTER)) {
        struct type* base = 0;
        if (!TypeSpec(&base))
            fatal("expected type-spec\n");

        struct type* newtyp = emalloc(sizeof(*newtyp));
        *newtyp = (struct type){
            .kind = TYPE_POINTER,
            .pointer.base = base,
        };
        *spec = newtyp;

        return true;
    } else if (Accept(RECORD)) {
        struct type* base = 0;
        if (Accept(LPAREN)) {
            if (!TypeSpec(&base))
                fatal("expected type-spec\n");
            Expect(RPAREN);
        }

        struct binds* fields = 0;
        while (1) {
            char fieldnam[IDENT_BUF_SIZE] = {0};
            if (!Identifier(fieldnam))
                break;

            struct binds* newfield = emalloc(sizeof(*newfield));
            *newfield = (struct binds){.prev = fields};
            strcpy(newfield->name, fieldnam);
            if (!TypeSpec(&newfield->type))
                fatal("expected type-spec\n");
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
    if (!Accept(TYPE))
        return false;

    char typnam[IDENT_BUF_SIZE] = {0};
    if (!Identifier(typnam))
        fatal("expected type name\n");

    struct type* typspec = 0;
    if (!TypeSpec(&typspec))
        fatal("expected type-spec\n");

    struct symtab* bound = intern(typnam, KLASS_TYPE);
    bound->type.spec = typspec;

    describe_type(1, bound->type.spec);
    printf("size of type: %zd\n", type_size_of(bound->type.spec));

    return true;
}

static bool VarDecl(void) {
    if (!Accept(VAR))
        return false;

    char varnam[IDENT_BUF_SIZE] = {0};
    if (!Identifier(varnam))
        fatal("expected identifier\n");

    struct type* thetype = 0;
    if (!TypeSpec(&thetype))
        fatal("expected type-spec\n");

    struct symtab* bound = intern(varnam, KLASS_VAR);
    bound->var.type = thetype;

    return true;
}

static bool ConstDecl(void) {
    if (!Accept(CONST))
        return false;

    char constnam[IDENT_BUF_SIZE] = {0};
    if (!Identifier(constnam))
        fatal("expected ident\n");

    long expr = 0;
    if (!ConstExpr(&expr))
        fatal("expected const-expr\n");

    struct symtab* bound = intern(constnam, KLASS_CONST);
    bound->constant.valu = expr;

    return true;
}

static bool FuncDecl(void) {
    if (!Accept(FUNC))
        return false;

    char funcnam[IDENT_BUF_SIZE] = {0};
    if (!Identifier(funcnam))
        fatal("expected ident\n");

    size_t arity = 0;
    struct binds* params = 0;

    if (Accept(LPAREN)) {
        do {
            char paramnam[IDENT_BUF_SIZE] = {0};
            if (!Identifier(paramnam))
                fatal("expected param identifier\n");

            struct type* type = 0;
            if (!TypeSpec(&type))
                fatal("expected type-spec\n");

            struct binds* newparam = emalloc(sizeof(*newparam));
            *newparam = (struct binds){};
            strcpy(newparam->name, paramnam);
            newparam->type = type;

            ++arity;
        } while (looksym != RPAREN);
        lex_getsym(); // consumed ')'
    }

    struct type* rettype = 0;
    if (!TypeSpec(&rettype))
        fatal("expected type-spec\n");

    struct symtab* newfunc = intern(funcnam, KLASS_FUNC);
    newfunc->func.arity = arity;
    newfunc->func.params = params;
    newfunc->func.rettype = rettype;

    // TODO body

    Expect(END);

    return true;
}

static bool Decl(void) {
    return ConstDecl() || FuncDecl() || VarDecl() || TypeDecl();
}

int main(int argc, char** argv) {
    enter_scope();
    lex_init();
    while (Decl());
    leave_scope();
    return 0;
}
