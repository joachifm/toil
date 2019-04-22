#include <stdlib.h>
#include <sys/syscall.h>
#include "aux.hh"

#ifdef PARSER_TEST_MAIN
#include "codegen.cc"
#include "scanner.cc"
#endif

namespace parser {

void Expression();

auto Factor() {
    if (scan::sym == '#') {
        auto num = scan::get_number();
        printf("    movl $%d,%%eax\n", num);
    } else if (scan::sym == 'x') {
        char varnam[scan::token_buf_siz];
        scan::get_name(varnam);
        printf("    movl $42,%%eax\n");
    } else if (scan::accept('(')) {
        while (!scan::accept(')'))
            Expression();
    } else {
        error("expected factor, got %c", scan::sym);
    }
}

void Expression() {
    Factor();                                 // lhs now in %eax
    while (scan::sym == '+') {
        if (scan::accept('+')) {
            printf("    movl %%eax,%%edx\n"); // lhs now in %edx
            Factor();                         // rhs now in %eax
            printf("    addl %%edx,%%eax\n"); // result in %eax
        }
    }
}

void Block(); // forward

auto IfElse() {
    auto l1 = codegen::next_label();
    auto l2 = codegen::next_label();
    scan::match('I');
    Expression();                     // condition result in %%eax
    printf("    test %%eax,%%eax\n"); // aka cmp $0,%eax
    printf("    jz %s\n", l1);        // condition evals to 0, jump to else
    Block();                          // then-branch code
    printf("    jmp %s\n", l2);       // condition was true, step over else
    scan::match_string("ELSE");       // full string to enforce syntax
    printf("%s:\n", l1);
    Block();                          // else-branch code
    scan::match_string("ENDIF");
    printf("%s:\n", l2);
}

auto While() {
    auto l1 = codegen::next_label();
    auto l2 = codegen::next_label();
    scan::match('W');
    printf("%s:\n", l1);
    Expression(); // condition in %eax
    printf("    test %%eax,%%eax\n");
    printf("    jz %s\n", l2); // condition evals to 0, end
    Block();
    scan::match_string("ENDWHILE");
    printf("    jmp %s\n", l1); // next iteration
    printf("%s:\n", l2);
}

auto Assignment() {
    char varnam[scan::token_buf_siz];
    scan::get_name(varnam);
    scan::match(':'); scan::match('=');
    Expression();
}

void Block() {
    while (scan::sym != 'E') {
        if      (scan::sym == 'I') IfElse();
        else if (scan::sym == 'W') While();
        else if (scan::sym == 'x') Assignment();
        else error("expected statement");
    }
}

auto Program() {
    scan::match_string("PROGRAM");
    char prognam[scan::token_buf_siz];
    scan::get_name(prognam);

    printf("    .text\n");
    printf("    .globl main\n");
    printf("main:\n");

    Block();

    printf("\n");
    printf("    movl $%d,%%eax\n", SYS_exit);
    printf("    xorl %%edi,%%edi\n");
    printf("    syscall\n");
}

} // namespace parser

#ifdef PARSER_TEST_MAIN
int main() {
    using namespace parser;
    scan::init();
    Program();
}
#endif
