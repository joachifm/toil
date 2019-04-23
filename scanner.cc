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

    // Skip leading whitespace
    while (isspace(look)) look = getchar();

    // Classify token
    if (isdigit(look)) { // Numeric constant
        sym = '#';
    } else if (islower(look)) { // Identifier
        sym = 'x';
    } else {
        sym = look;
    }

    // Read token value
    //
    // Note: only numbers and identifiers/keywords
    // get multichar tokens ...
    //
    // An alternative is to consume upto next space but then
    // e.g., "(1" would be parsed as a single token.
    len = 0;
    do { val[len++] = static_cast<char>(look); look = getchar(); }
    while (len < token_len_max && (isalnum(look) || look == '_'));
    val[len] = '\0';
}

// Advance token if current sym matches or abort.
auto match(int c) {
    if (sym != c) error("expected sym='%c'", c);
    get_sym();
}

// Advance token if current lexeme matches or abort.
auto match_string(char const* s) {
    if (strcmp(val, s) != 0) error("expected '%s'", s);
    get_sym();
}

auto accept(int c) {
    if (sym != c) return false;
    get_sym();
    return true;
}

// Extract name or abort.
auto get_name(char* out) {
    if (sym != 'x') error("expected name");
    strcpy(out, val);
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
