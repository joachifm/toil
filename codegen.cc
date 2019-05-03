#pragma once

#include "aux.hh"
#include "labels_gen.h"

namespace codegen {

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

auto next_label() -> char const* {
    static unsigned next;
    if (next > label_count_max - 1) error("exhausted labels");
    unsigned ind = next++;
    return labels[ind];
}

} // namespace codegen

namespace cgen = codegen;
