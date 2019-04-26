#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

// To avoid dynamic memory allocation, parsing routines receive pointers to Item
// storage from their caller (allocated on caller's stack).
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
        CON = 2,
        REG = 4,
        VAR = 8,
    } t;

    union {
        struct { int val; } con;
        struct { int idx; } reg;
        struct { int adr; } var;
    };
};

typedef struct item Item;

#define GET_CON(X) \
    ({ assert((X)->t == CON); (X)->con.val; })
#define GET_VAR(X) \
    ({ assert((X)->t == VAR); (X)->var.adr; })
#define GET_REG(X) \
    ({ assert((X)->t == REG); (X)->reg.val; })

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
    [REG_EDX] = "edx",
    [REG_EBX] = "ebx",
    [REG_EDI] = "edi",
    [REG_ESI] = "esi",
};

#define REGNAM(I) \
    ({ assert((I) >= 1 && (I) < NUM_REG); regnam[I]; })

static void print_item(Item const* it) {
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
    while (sym == '*' || sym == '/') {
        // XXX copy-paste galore
        if (accept('*')) {
            Item* h = factor(&(Item){});
            if (r->t == CON && h->t == CON) {
                r->con.val *= h->con.val;
            }
        } else if (accept('/')) {
            Item* h = factor(&(Item){});
            if (r->t == CON && h->t == CON) {
                r->con.val /= h->con.val;
            }
        }
    }
    return r;
}

Item* arith_expression(Item * r) {
    term(r);
    while (sym == '+' || sym == '-') {
        // XXX copy-paste galore
        if (accept('+')) {
            Item* h = term(&(Item){});
            if (r->t == CON && h->t == CON) {
                r->con.val += h->con.val;
            }
        } else if (accept('-')) {
            Item* h = term(&(Item){});
            if (r->t == CON && h->t == CON) {
                r->con.val -= h->con.val;
            }
        }
    }
    return r;
}

Item* expression(Item * r) {
    arith_expression(r);
    while (sym == '>' || sym == '<' || sym == '=') {
        // XXX copy-paste galore
        if (accept('>')) {
            Item* h = arith_expression(&(Item){});
            if (r->t == CON && h->t == CON) {
                r->con.val = r->con.val > h->con.val;
            }
        } else if (accept('=')) {
            Item* h = arith_expression(&(Item){});
            if (r->t == CON && h->t == CON) {
                r->con.val = r->con.val == h->con.val;
            }
        } else if (accept('<')) {
            Item* h = arith_expression(&(Item){});
            if (r->t == CON && h->t == CON) {
                r->con.val = r->con.val < h->con.val;
            }
        }
    }
    return r;
}

int main(void) {
    sym = getchar();
    Item* r = expression(&(Item){});
    print_item(r);
}
