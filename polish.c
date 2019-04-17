#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* Size of buffer holding token including NUL. */
#define TOKEN_BUF_SIZ 16

struct dict {
    char name[TOKEN_BUF_SIZ];
    struct dict* prev;
};

static int look; /* Character lookahead; holds first char after last sym (if any) */
static char tok[TOKEN_BUF_SIZ]; /* Last token value */
static int sym; /* Last token code */
static struct dict* last;

#define PANIC_ON(EXPR) if (!(EXPR)) { ; } else { abort(); }

static int scan(void) {
    while (isspace(look)) { look = getchar(); }
    sym = look;
    tok[0] = '\0';
    if (look == EOF) goto out;
    size_t len = 0;
    do { tok[len++] = (char)look; look = getchar(); }
    while (len < TOKEN_BUF_SIZ - 1 && !isspace(look));
    tok[len] = '\0';
out:
    return sym;
}

int main(void) {
    look = getchar();
    while (scan() != EOF) {
        if (isdigit(sym)) {
            puts("<literal>");
        } else if (sym == '+') {
            puts("<primop-add>");
        } else if (sym == '-') {
            puts("<primop-sub>");
        } else if (sym == '>' || sym == '<' || sym == '=') {
            puts("<primop-rel>");
        } else if (sym == ',') {
            scan(); /* Name of entry */
            struct dict* ent = malloc(sizeof(*ent));
            PANIC_ON(!ent);
            *ent = (struct dict){.prev = last};
            strcpy(ent->name, tok);
            last = ent;
            printf("created %s\n", last->name);
        } else {
            printf("CALL %s\n", tok);
        }
    }
}
