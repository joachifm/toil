#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

// A parsing procedure processes a language construct and returns an item
// describing the value of the construct.
//
// Parsing proceeds top-down, items are propagated bottom-up
//
// Code is generated only if the transformation cannot be completed at compile
// time, at the last possible moment.

// To avoid allocation, parsing routines receive pointers to Item storage from
// their caller.
//
// The pattern
//
//    Item* r = parser(&(Item){});
//
// is preferred over
//
//    Item r = {0};
//    parser(&r);
//
// as it is more compact and ensures initialization (cannot pass an
// uninitialized temporary).

enum {
    REG_EAX = 1,
    REG_EBP,
    REG_EBX,
    REG_EDX,

    REG_EDI,
    REG_ESI,

    NUM_REG,
};

struct item {
    enum {
        CON = 1,
        REG,
        VAR,
    } t;

    union {
        struct { int val; } con;
        struct { int adr; } var;
        struct { int idx; } reg;
    };
};

typedef struct item Item;

static int sym = 0;

static int match(int c) {
    return sym == c;
}

static int accept(int c) {
    if (sym != c) return 0;
    sym = getchar();
    return 1;
}

static int expect(int c) {
    if (!accept(c)) abort();
}

static int number(void) {
    if (!isdigit(sym)) abort();
    int n = atoi((char[2]){(char)sym, '\0'});
    sym = getchar();
    return n;
}

static char regnam[NUM_REG][4] = {
    [REG_EAX] = "eax",
};

#define REGNAM(I) \
    ({ assert((I) >= 1 && (I) < NUM_REG); regnam[I]; })

static void pr_item(Item const* it) {
    printf("Item {");
    switch (it->t) {
    case CON:
        printf("type: CON, val: %d", it->con.val);
        break;
    case REG:
        printf("type: REG, nam: %s", REGNAM(it->reg.idx));
        break;
    case VAR:
        printf("type: VAR, adr: %d", it->var.adr);
        break;
    }
    printf("}\n");
}

// An expression
//
//   a + b + ... n
//
// is eval'd left to right, with the result accumulated in
// eax.

static Item* emit_arith_oper(char const* op, Item* r) {
    printf("    %s ", op);
    switch (r->t) {
    case CON:
        printf("$%d", r->con.val);
        break;
    case VAR:
        printf("%d(%%rip)", r->var.adr);
        break;
    case REG:
        printf("%%%s\n", REGNAM(r->reg.idx));
        break;
    }
    printf(",%%eax\n");
    return r;
}

static Item* expression(Item* r);

static Item* variable(Item* r) {
    sym = getchar();
    *r = (Item){ .t = VAR, .var.adr = 64, };
    return r;
}

static Item* constant(Item* r) {
    *r = (Item){ .t = CON, .con.val = number(), };
    return r;
}

static Item* factor(Item* r) {
    if (isdigit(sym)) {
        constant(r);
    } else if (islower(sym)) {
        variable(r);
    } else if (accept('(')) {
        while (!accept(')'))
            expression(r);
    } else {
        abort();
    }
    return r;
}

static Item* term(Item* r) {
    factor(r);
    while (sym == '*') {
        if (accept('*')) {
            Item* h = factor(&(Item){});
            if (r->t == CON && h->t == CON) {
                r->con.val *= h->con.val;
            } else {
                emit_arith_oper("mul", r);
                emit_arith_oper("mul", h);
                *r = (Item){ .t = REG, .reg.idx = REG_EAX };
            }
        }
    }
    return r;
}

Item* arith_expression(Item * r) {
    term(r);
    while (sym == '+') {
        if (accept('+')) {
            Item* h = term(&(Item){});
            if (r->t == CON && h->t == CON) {
                r->con.val += h->con.val;
            } else {
                emit_arith_oper("add", r);
                emit_arith_oper("add", h);
                *r = (Item){ .t = REG, .reg.idx = REG_EAX };
            }
        }
    }
    return r;
}

Item* expression(Item * r) {
    arith_expression(r);
    while (sym == '>' || sym == '<' || sym == '=') {
        if (accept('>')) {
            Item* h = arith_expression(&(Item){});
            if (r->t == CON && h->t == CON) {
                int t1 = r->con.val > h->con.val;
                r->con.val = t1;
            } else {
                emit_arith_oper("cmp", r);
                emit_arith_oper("cmp", h);
                *r = (Item){ .t = REG, .reg.idx = REG_EAX };
            }
        } else if (accept('=')) {
            Item* h = arith_expression(&(Item){});
            if (r->t == CON && h->t == CON) {
                int t1 = r->con.val == h->con.val;
                r->con.val = t1;
            } else {
                emit_arith_oper("cmp", r);
                emit_arith_oper("cmp", h);
                *r = (Item){ .t = REG, .reg.idx = REG_EAX };
            }
        } else if (accept('<')) {
            Item* h = arith_expression(&(Item){});
            if (r->t == CON && h->t == CON) {
                int t1 = r->con.val < h->con.val;
                r->con.val = t1;
            } else {
                emit_arith_oper("cmp", r);
                emit_arith_oper("cmp", h);
                *r = (Item){ .t = REG, .reg.idx = REG_EAX };
            }
        }
    }
    return r;
}

int main(void) {
    sym = getchar();
    Item* r = expression(&(Item){});
    pr_item(r);
}
