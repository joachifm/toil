#pragma once

#include "aux.hh"

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

} // namespace codegen

namespace cgen = codegen;
