#pragma once

#include <cstdlib>
#include <sys/syscall.h>

namespace parser {

void Expression();

auto Factor() {
    if (scan::sym == '#') {
        auto num = scan::get_number();
        printf("    movl $%d,%%edx\n", num);
    } else if (scan::sym == 'x') {
        char varnam[scan::token_buf_siz];
        scan::get_name(varnam);
        printf("    movl %s(%%eip),%%edx\n", varnam);
    } else if (scan::accept('(')) {
        // TODO will inner Expression clobber registers in outer Expression?
        while (!scan::accept(')'))
            Expression();
    } else {
        error("expected factor, got %c", scan::sym);
    }
}

void Expression() {
    Factor();
    // TODO unnecessary shuffle if no arith operation
    printf("    movl %%edx,%%eax\n");
    while (scan::sym == '+' || scan::sym == '-') {
        if (scan::accept('+')) {
            Factor();
            printf("    addl %%edx,%%eax\n");
        } else if (scan::accept('-')) {
            Factor();
            printf("    subl %%edx,%%eax\n");
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

auto Loop() {
    scan::match('L');

    char varnam[scan::token_buf_siz];
    scan::get_name(varnam);

    scan::match_string("FROM");
    auto from = scan::get_number();

    scan::match_string("TO");
    auto upto = scan::get_number();

    if (from > upto) error("LOOP expects UPTO >= FROM");
    auto n_iter = upto - from;
    // TODO emit nothing if upto - from = 0 ?
    //      need to be able to still parse the remainder, just
    //      not emit anything
    // TODO if n_iter=0, the generated code is an endless loop
    if (n_iter < 1) {
        fprintf(stderr, "warning: 0 iteration loop; emitting dead code\n");
    }

    // TODO scoped loop var?
    printf("    .data\n");
    printf("%s: .int %d\n", varnam, n_iter);
    printf("    .text\n");

    auto l1 = codegen::next_label();
    printf("    movl $%d,%%ecx\n", n_iter); // init loop counter
    printf("%s:\n", l1); // loop start
    // TODO somehow make loop var alias ecx without movl every go around?
    printf("    movl %%ecx,%s(%%eip)\n", varnam);
    Block();
    printf("    loop %s\n", l1); // recur when counter = 0

    scan::match_string("ENDLOOP");
}

auto Assignment() {
    char varnam[scan::token_buf_siz];
    scan::get_name(varnam);
    scan::match(':'); scan::match('=');
    Expression();
    printf("    movl %%eax,%s(%%eip)\n", varnam);
}

void Block() {
    while (scan::sym != 'E') {
        if      (scan::sym == 'I') IfElse();
        else if (scan::sym == 'W') While();
        else if (scan::sym == 'x') Assignment();
        else if (scan::sym == 'L') Loop();
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
