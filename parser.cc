#pragma once

#include <cstdio>
#include <cstdlib>
#include <sys/syscall.h>

#include "aux.hh"
#include "codegen.cc"
#include "scanner.cc"

namespace parser {

void Expression();

auto Factor() {
    if (scan::sym == '#') {
        auto num = scan::get_number();
        printf("    pushq $%d\n", num);
    } else if (scan::sym == 'x') {
        char varnam[scan::token_buf_siz];
        scan::get_name(varnam);
        printf("    pushq %s(%%eip)\n", varnam);
    } else if (scan::accept('(')) {
        while (!scan::accept(')'))
            Expression();
    } else {
        error("expected factor, got %c", scan::sym);
    }
}

auto Term() {
    Factor();
    while (scan::sym == '*' || scan::sym == '/') {
        if (scan::accept('*')) {
            Factor();
            printf("    popq %%rdx\n");
            printf("    popq %%rax\n");
            printf("    imul %%edx,%%eax\n");
            printf("    pushq %%rax\n");
        } else if (scan::accept('/')) {
            Factor();
            printf("    popq %%rcx\n"); // divisor
            printf("    popq %%rax\n"); // dividend
            printf("    xorl %%edx,%%edx\n"); // dividend stored over rdx:rax, we only use rax
            printf("    idiv %%ecx\n");
            printf("    pushq %%rdx\n"); // quotient
            printf("    pushq %%rax\n"); // result
        }
    }
}

auto ArithExpression() {
    Term();
    while (scan::sym == '+' || scan::sym == '-') {
        if (scan::accept('+')) {
            Term();
            printf("    popq %%rdx\n");
            printf("    popq %%rax\n");
            printf("    addl %%edx,%%eax\n");
            printf("    pushq %%rax\n");
        } else if (scan::accept('-')) {
            Term();
            printf("    popq %%rdx\n");
            printf("    popq %%rax\n");
            printf("    subl %%edx,%%eax\n");
            printf("    pushq %%rax\n");
        }
    }
}

auto Relation() {
    ArithExpression();
    while (scan::sym == '>' || scan::sym == '<' || scan::sym == '=') {
        if (scan::accept('>')) {
            ArithExpression();
            printf("    popq %%rdx\n"); // b
            printf("    popq %%rax\n"); // a
            printf("    cmp %%edx,%%eax\n");
            printf("    setg %%al\n");
            printf("    movzx %%al,%%eax\n");
            printf("    pushq %%rax\n");
        } else if (scan::accept('<')) {
            ArithExpression();
            printf("    popq %%rdx\n"); // b
            printf("    popq %%rax\n"); // a
            printf("    cmp %%edx,%%eax\n");
            printf("    setl %%al\n");
            printf("    movzx %%al,%%eax\n");
            printf("    pushq %%rax\n");
        } else if (scan::accept('=')) {
            ArithExpression();
            printf("    popq %%rdx\n"); // b
            printf("    popq %%rax\n"); // a
            printf("    cmp %%edx,%%eax\n");
            printf("    sete %%al\n");
            printf("    movzx %%al,%%eax\n");
            printf("    pushq %%rax\n");
        }
    }
}

void Expression() {
    Relation();
    while (scan::sym == 'A' || scan::sym == 'O') {
        if (scan::accept('A')) {
            char l1[cgen::label_buf_siz];
            char l2[cgen::label_buf_siz];
            cgen::next_label(l1);
            cgen::next_label(l2);
            printf("    popq %%rax\n");
            printf("    test %%eax,%%eax\n");
            printf("    jz %s\n", l1); // lhs false, short-circuit
            Relation();
            printf("    jmp %s\n", l2); // lhs true, step over false case
            printf("%s:\n", l1); // in the false case, push zero to stack
            printf("    sete %%al\n");
            printf("    movzx %%al,%%eax\n");
            printf("    xorl %%eax,%%eax\n"); // eax 1 from sete, xor to 0
            printf("    pushq %%rax\n");
            printf("%s:\n", l2);
        } else if (scan::accept('O')) {
            char l1[cgen::label_buf_siz];
            cgen::next_label(l1);
            printf("    popq %%rax\n");
            printf("    test %%eax,%%eax\n");
            printf("    pushq %%rax\n"); // retain lhs
            printf("    jnz %s\n", l1); // lhs was true; step over rhs
            Relation();
            printf("%s:\n", l1);
        }
    }
}

void Block(); // forward

auto IfElse() {
    char l1[cgen::label_buf_siz];
    char l2[cgen::label_buf_siz];
    cgen::next_label(l1);
    cgen::next_label(l2);
    scan::match('I');
    Expression();                     // condition result top of stack
    printf("    popq %%rax\n");
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

auto RepeatUntil() {
    scan::match('R');
    char l1[cgen::label_buf_siz];
    cgen::next_label(l1);
    printf("%s:\n", l1);
    Block();
    scan::match_string("UNTIL");
    Expression();
    printf("    popq %%rax\n");
    printf("    test %%eax,%%eax\n");
    printf("    jz %s\n", l1); // recur until condition evals to 1
}

auto While() {
    char l1[cgen::label_buf_siz];
    char l2[cgen::label_buf_siz];
    cgen::next_label(l1);
    cgen::next_label(l2);
    scan::match('W');
    printf("%s:\n", l1);
    Expression(); // condition top of stack
    printf("    popq %%rax\n");
    printf("    test %%eax,%%eax\n");
    printf("    jz %s\n", l2); // condition evals to 0, end
    Block();
    scan::match_string("ENDWHILE");
    printf("    jmp %s\n", l1); // next iteration
    printf("%s:\n", l2);
}

auto ForLoop() {
    scan::match('F');

    char varnam[scan::token_buf_siz];
    scan::get_name(varnam);

    scan::match_string("FROM");
    auto from = scan::get_number();

    scan::match_string("TO");
    auto upto = scan::get_number();

    if (from >= upto) error("FOR expects UPTO > FROM");
    auto n_iter = upto - from;

    // TODO access loop counter in body

    printf("    movl $%d,%%ecx\n", n_iter);
    char l1[cgen::label_buf_siz];
    cgen::next_label(l1);
    printf("%s:\n", l1);
    Block();
    printf("    dec %%ecx\n");
    printf("    jne %s\n", l1);
    scan::match_string("ENDFOR");
}

auto DoTimes() {
    scan::match('T');
    auto n_iter = scan::get_number();
    if (n_iter < 1) error("TIMES expects n > 0");
    // TODO unroll if n_iter < threshold
    char l1[cgen::label_buf_siz];
    cgen::next_label(l1);
    // TODO unnecessary push/pop if not nested within another loop
    printf("    pushq %%rcx\n");
    printf("    mov $%d,%%ecx\n", n_iter);
    printf("%s:\n", l1);
    Block();
    printf("    loop %s\n", l1);
    scan::match_string("ENDTIMES");
    printf("    popq %%rcx\n");
}

auto Assignment() {
    char varnam[scan::token_buf_siz];
    scan::get_name(varnam);
    scan::match(':'); scan::match('=');
    Expression();
    printf("    popq %s(%%eip)\n", varnam);
}

void Block() {
    while (scan::sym != 'E' && scan::sym != 'U') {
        if      (scan::sym == 'I') IfElse();
        else if (scan::sym == 'W') While();
        else if (scan::sym == 'x') Assignment();
        else if (scan::sym == 'F') ForLoop();
        else if (scan::sym == 'T') DoTimes();
        else if (scan::sym == 'R') RepeatUntil();
        else error("expected statement");
    }
}

auto VarDecl() {
    scan::match_string("VAR");

    char varnam[scan::token_buf_siz];
    scan::get_name(varnam);

    scan::match_string("INT");

    printf("%s: .int 0\n", varnam);
}

auto Program() {
    scan::match_string("PROGRAM");
    char prognam[scan::token_buf_siz];
    scan::get_name(prognam);

    printf("    .data\n");
    while (scan::sym == 'V')
        VarDecl();
    printf("\n");

    printf("    .global _start\n");
    printf("    .text\n");
    printf("_start:\n");

    Block();

    printf("_end_of_program:\n"); // marker to identify end of emitted code

    printf("\n");
    printf("    movl $%d,%%eax\n", SYS_exit);
    printf("    xorl %%edi,%%edi\n");
    printf("    syscall\n");
}

auto compile() {
    scan::init();
    Program();
}

} // namespace parser

#ifdef PARSER_TEST_MAIN
int main() {
    using namespace parser;
    compile();
}
#endif
