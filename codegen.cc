#include <cstdio>
#include "aux.hh"
#include "labels_gen.h"

namespace codegen {

auto next_label() -> char const* {
    static unsigned next;
    if (next > label_count_max - 1) error("exhausted labels");
    unsigned ind = next++;
    return labels[ind];
}

} // namespace codegen
