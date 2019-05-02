#pragma once

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "aux.hh"

namespace scan {

constexpr size_t token_len_max = 15;
constexpr size_t token_buf_siz = token_len_max + 1;

int look; // Lookahead
int sym; // Token code
char val[token_buf_siz]; // Token value (aka. the lexeme)
size_t len; // Token value length

// Post-conditions:
// - val, len, and sym describe the current token
// - look holds first char not part of the current token
auto get_sym() {
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
auto match(int c) {
    if (sym != c) error("expected sym '%c'; got '%c' (val='%s')", c, sym, val);
    get_sym();
}

// Advance token if current lexeme matches or abort.
auto match_string(char const* s) {
    assert(s);
    if (strcmp(val, s) != 0) error("expected '%s'; got '%s'", s, val);
    get_sym();
}

auto accept(int c) {
    if (sym != c) return false;
    get_sym();
    return true;
}

// Extract name or abort.
auto get_name(char* out) {
    assert(out);
    if (sym != 'x') error("expected name");
    strncpy(out, val, token_len_max);
    get_sym();
}

// Extract number or abort.
auto get_number() {
    if (sym != '#') error("expected number");
    auto num = atoi(val);
    get_sym();
    return num;
}

auto is_eof() {
    return look == EOF;
}

auto init() {
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
