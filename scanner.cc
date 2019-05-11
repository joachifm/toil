#pragma once

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "aux.hh"
#include "scanner.hh"

namespace scanner {

// extern
int look;
int sym;
char val[token_buf_siz];
size_t len;

// Post-conditions:
// - val, len, and sym describe the current token
// - look holds first char not part of the current token
auto get_sym() -> void {
    assert(look != EOF);

    while (isspace(look)) look = getchar();

    if (isdigit(look)) { // Numeric constant
        sym = '#';
        len = 0;
        do { val[len++] = static_cast<char>(look); look = getchar(); }
        while (len < token_len_max && isdigit(look));
        val[len] = '\0';
    } else if (islower(look)) { // Identifier
        sym = 'x';
        len = 0;
        do { val[len++] = static_cast<char>(look); look = getchar(); }
        while (len < token_len_max && (isalnum(look) || look == '_'));
        val[len] = '\0';
    } else if (isupper(look)) { // Keyword
        sym = look;
        len = 0;
        do { val[len++] = static_cast<char>(look); look = getchar(); }
        while (len < token_len_max && isupper(look));
        val[len] = '\0';
    } else { // Special
        sym = look;
        len = 1;
        val[0] = static_cast<char>(look);
        val[1] = '\0';
        look = getchar();
    }
}

// Advance token if current sym matches or abort.
auto match(int c) -> void {
    if (sym != c) error("expected sym '%c'; got '%c' (val='%s')", c, sym, val);
    get_sym();
}

// Advance token if current lexeme matches or abort.
auto match_string(char const* s) -> void {
    assert(s);
    if (!STREQ(val, s)) error("expected '%s'; got '%s'", s, val);
    get_sym();
}

auto accept(int c) -> bool {
    if (sym != c) return false;
    get_sym();
    return true;
}

// Extract name or abort.
auto get_name(char* out) -> void {
    assert(out);
    if (sym != 'x') error("expected name");
    strncpy(out, val, sizeof(val)-1);
    get_sym();
}

// Extract number or abort.
auto get_number() -> int {
    if (sym != '#') error("expected number");
    auto num = atoi(val);
    get_sym();
    return num;
}

auto is_eof() -> bool {
    return look == EOF;
}

auto init() -> void {
    look = getchar();
    get_sym();
}

} // namespace scanner

#ifdef SCANNER_TEST_MAIN
int main() {
    scan::init();
    while (!scan::is_eof()) {
        scan::get_sym();
        printf("sym='%c', val='%s'\n", scan::sym, scan::val);
    }
}
#endif
