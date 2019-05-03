#pragma once

#include <cstring>

#include "aux.hh"
#include "scanner.cc"

namespace codegen {

constexpr size_t label_buf_siz = 11;

#define LABEL(VAR) char VAR[codegen::label_buf_siz]; codegen::next_label(VAR);

auto next_label(char* buf) -> char* {
    static unsigned next = 1; // 0 means we overflowed
    if (next == 0) error("exhausted labels");
    snprintf(buf, label_buf_siz, "L%u", next);
    ++next;
    return buf;
}

struct Symtab {
    char name[scan::token_buf_siz];

    Symtab* prev = nullptr;

    enum Klass : unsigned {
        KLASS_ANY = 0, // not a valid klass, catch-all for lookups
        KLASS_CON,
        KLASS_VAR,
    } t;

    union {
        // when KLASS_CON
        struct { int val; } con;

        // when KLASS_VAR
        struct { int adr; } var;
    };
};

constexpr size_t symtab_max_ent = 1000;

Symtab* symtab = nullptr;

auto intern(char const* name, Symtab::Klass klass) {
    static Symtab symtab_slab[symtab_max_ent] = {};
    static size_t symtab_slot = 0;

    if (symtab_slot > symtab_max_ent - 1) error("exhausted symbols");
    size_t slot = symtab_slot++;

    Symtab* ent = &symtab_slab[slot];
    strcpy(ent->name, name);
    ent->t = klass;

    ent->prev = symtab;
    symtab = ent;
}

auto same_klass(Symtab::Klass want, Symtab::Klass other) {
    if (want == Symtab::KLASS_ANY) return true;
    return want == other;
}

auto resolve(char const* name, Symtab::Klass kind) {
    Symtab* p = symtab;
    for (; p && !(same_klass(kind, p->t) && STREQ(p->name, name)) ; p = p->prev);
    return p;
}

auto print_symtab() {
    fprintf(stderr, "name\tklass\n");
    fprintf(stderr, "----\t-----\n");
    for (Symtab* p = symtab; p; p = p->prev) {
        fprintf(stderr, "%s\t%d\n", p->name, p->t);
    }
}

struct Item {
    enum Kind : unsigned {
        KIND_CON,
        KIND_REG,
        KIND_VAR,
    } t;

    union {
        // when KIND_CON
        struct { int val; } con;

        // when KIND_REG
        struct { int a; int b; } reg;

        // when KIND_VAR
        struct { int adr; } var;
    };
};

} // namespace codegen

namespace cgen = codegen;

#ifdef SYMTAB_TEST_MAIN
#undef NDEBUG
#include <cassert>
#include <cstdio>

int main() {
    using namespace cgen;
    intern("foo", Symtab::Klass::KLASS_CON);
    assert(resolve("foo", Symtab::Klass::KLASS_CON));
    assert(resolve("foo", Symtab::Klass::KLASS_ANY));
    assert(!resolve("foo", Symtab::Klass::KLASS_VAR));
    assert(!resolve("bar", Symtab::Klass::KLASS_VAR));
}
#endif
