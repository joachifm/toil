#pragma once

#include <cstring>

#include "aux.hh"
#include "scanner.hh"
#include "symtab.hh"

namespace symtab {

namespace {

Symtab* symtab = nullptr;
int level = 0;

} // namespace

auto enter_scope() -> void {
    ++level;
}

auto leave_scope() -> void {
    if (level < 1) return;
    --level;
}

auto resolve(char const* name, Klass klass = Klass::any) -> Symtab* {
    Symtab* p = symtab;
    for (; p && !(p->level <= level
                    && (klass == Klass::any || klass == p->klass)
                    && STREQ(p->name, name));
         p = p->prev);
    return p;
}

auto intern(char const* name, Klass klass) -> Symtab* {
#define symtab_max_ent 1000
    static Symtab symtab_slab[symtab_max_ent] = {};
    static size_t symtab_slot = 0;

    // TODO consider any slot below current level as "free"
    if (symtab_slot > symtab_max_ent - 1) error("exhausted symbols");
    size_t slot = symtab_slot++;
#undef symtab_max_ent

    if (Symtab* exists = resolve(name, Klass::any); exists) {
        fprintf(stderr, "warn: shadowing existing decl '%s' (%d)\n",
                        exists->name, static_cast<int>(exists->klass));
    }

    Symtab* ent = &symtab_slab[slot];

    strcpy(ent->name, name);
    ent->klass = klass;
    ent->level = level;

    ent->prev = symtab;
    symtab = ent;

    return ent;
}

auto print_all() -> void {
    fprintf(stderr, "name\tklass\tscope\n");
    fprintf(stderr, "----\t-----\t-----\n");
    for (Symtab* p = symtab; p; p = p->prev) {
        fprintf(stderr, "%s\t%d\t%d\n", p->name, static_cast<int>(p->klass), p->level);
    }
}

} // namespace symtab

#ifdef SYMTAB_TEST_MAIN
#undef NDEBUG
#include <cassert>

int main() {
    using namespace symtab;

    intern("foo", Klass::con);

    enter_scope();
    intern("foo", Klass::var);
    enter_scope();
    assert(resolve("foo", Klass::var));
    leave_scope();
    assert(resolve("foo", Klass::var));
    leave_scope();
    assert(!resolve("foo", Klass::var));

    assert(resolve("foo", Klass::con));
    assert(resolve("foo", Klass::any));
    assert(resolve("foo"));
    assert(!resolve("bar", Klass::any));

    print_all();
}
#endif // ifdef SYMTAB_TEST_MAIN
