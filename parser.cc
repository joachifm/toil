#pragma once

#include <cstdio>
#include <cstdlib>
#include <sys/syscall.h>

#include "aux.hh"
#include "codegen.hh"
#include "parser.hh"
#include "scanner.hh"
#include "symtab.hh"

namespace {

auto Expression() -> void; // forward

auto Factor() {
    if (scan::sym == '#') {
        auto num = scan::get_number();
        printf("    pushq $%d\n", num);
    } else if (scan::sym == 'x') {
        char varnam[scan::token_buf_siz];
        scan::get_name(varnam);
#ifndef TEST
        if (!sym::resolve(varnam, sym::Klass::var))
            error("unbound variable: %s", varnam);
#endif
        printf("    pushq %s(%%eip)\n", varnam);
    } else if (scan::accept('(')) {
        while (!scan::accept(')'))
            Expression();
    } else {
        scan::expected("factor");
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

auto Expression() -> void {
    Relation();
    while (scan::sym == 'A' || scan::sym == 'O') {
        if (scan::accept('A')) {
            auto l1 = cgen::next_label();
            auto l2 = cgen::next_label();
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
            auto l1 = cgen::next_label();
            printf("    popq %%rax\n");
            printf("    test %%eax,%%eax\n");
            printf("    pushq %%rax\n"); // retain lhs
            printf("    jnz %s\n", l1); // lhs was true; step over rhs
            Relation();
            printf("%s:\n", l1);
        }
    }
}

auto Block() -> void; // forward

auto IfElse() {
    auto l1 = cgen::next_label();
    auto l2 = cgen::next_label();
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
    auto l1 = cgen::next_label();
    printf("%s:\n", l1);
    Block();
    scan::match_string("UNTIL");
    Expression();
    printf("    popq %%rax\n");
    printf("    test %%eax,%%eax\n");
    printf("    jz %s\n", l1); // recur until condition evals to 1
}

auto While() {
    auto l1 = cgen::next_label();
    auto l2 = cgen::next_label();
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
    auto l1 = cgen::next_label();
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
    auto l1 = cgen::next_label();
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
    if (scan::sym == ':') {
#ifndef TEST
        if (!sym::resolve(varnam, sym::Klass::var))
            error("assigning to undeclared variable: %s", varnam);
#endif
        scan::match(':'); scan::match('=');
        Expression();
        printf("    popq %s(%%eip)\n", varnam);
    } else if (scan::sym == '(') {
#ifndef TEST
        if (!sym::resolve(varnam, sym::Klass::proc))
            error("call to undeclared procedure: %s", varnam);
#endif
        scan::match('(');
        scan::match(')');
        printf("    call %s\n", varnam);
    } else {
        scan::expected("assignment or procedure call");
    }
}

auto Block() -> void {
    while (scan::sym != 'E' && scan::sym != 'U') {
        if      (scan::sym == 'I') IfElse();
        else if (scan::sym == 'W') While();
        else if (scan::sym == 'x') Assignment();
        else if (scan::sym == 'F') ForLoop();
        else if (scan::sym == 'T') DoTimes();
        else if (scan::sym == 'R') RepeatUntil();
        else scan::expected("statement");
    }
}

auto VarDecl() {
    scan::match_string("VAR");

    char varnam[scan::token_buf_siz];
    scan::get_name(varnam);

    sym::intern(varnam, sym::Klass::var);

    scan::match_string("INT");

    printf("%s: .int 0\n", varnam);
}

auto ProcDecl() {
    scan::match_string("PROC");
    char varnam[scan::token_buf_siz];
    scan::get_name(varnam);
    sym::intern(varnam, sym::Klass::proc);
    printf("%s:\n", varnam);
    Block();
    printf("    ret\n");
    scan::match_string("ENDPROC");
}

auto Program() {
    scan::match_string("PROGRAM");
    char prognam[scan::token_buf_siz];
    scan::get_name(prognam);

    printf("    .data\n");
    while (scan::sym == 'V')
        VarDecl();
    printf("\n");

    printf("    .text\n");

    while (scan::sym == 'P')
        ProcDecl();
    printf("\n");

    printf("    .global _start\n");
    printf("_start:\n");

    Block();

    printf("_end_of_program:\n"); // marker to identify end of emitted code

    printf("\n");
    printf("    movl $%d,%%eax\n", SYS_exit);
    printf("    xorl %%edi,%%edi\n");
    printf("    syscall\n");
}

} // namespace

namespace parser {

auto compile() -> void {
    scan::init();
    Program();
}

} // namespace parser

#ifdef PARSER_TEST_MAIN
int main() {
    parser::compile();
    sym::print_all();
}
#endif
