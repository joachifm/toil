#pragma once

#include "scanner.hh"

namespace symtab {

enum class Klass {
    any = 0, // not a valid klass, catch-all for lookups
    con,
    proc,
    var,
};

struct Symtab {
    char name[scan::token_buf_siz];
    int level = 0;
    Klass klass = Klass::any;
    Symtab* prev = nullptr;

    union {
        struct { int val; } con;

        struct { int adr; } var;

        struct { int adr; } proc;
    };
};

auto enter_scope() -> void;
auto leave_scope() -> void;

auto resolve(char const* name, Klass klass) -> Symtab*;
auto intern(char const* name, Klass klass) -> Symtab*;
auto print_all() -> void;

} // namespace symtab

namespace sym = symtab;
