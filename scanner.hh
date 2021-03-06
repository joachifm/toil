#pragma once

#include <cstddef> // size_t

namespace scanner {

constexpr size_t token_len_max = 15;
constexpr size_t token_buf_siz = token_len_max + 1;

extern int sym; // Token code

auto init() -> void;

auto get_name(char (&out)[token_buf_siz]) -> void;
auto get_number() -> int;
auto get_sym() -> void;

auto accept(int c) -> bool;
auto is_eof() -> bool;

auto match(int c) -> void;
auto match_string(char const* s) -> void;

[[noreturn]] auto expected(char const* what) -> void;

}

namespace scan = scanner;
