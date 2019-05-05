#pragma once

#include "aux.hh"

namespace codegen {

auto next_label() -> char const* {
#include "labels_gen.h"
    static unsigned next;
    if (next > label_count_max - 1) error("exhausted labels");
    unsigned ind = next++;
    return labels[ind];
}

} // namespace codegen

namespace cgen = codegen;
