



#ifdef DEF
DEF(This)
DEF(is)
DEF(a)
DEF(test)
DEF(of)
DEF(string)
DEF(array)
DEF(for)
DEF(use)
DEF(with)
DEF(elfhack)
DEF(to)
DEF(see)
DEF(whether)
DEF(it)
DEF(breaks)
DEF(anything)
DEF(but)
DEF(one)
DEF(needs)
DEF(quite)
DEF(some)
DEF(strings)
DEF(before)
DEF(the)
DEF(program)
DEF(can)
DEF(do)
DEF(its)
DEF(work)
DEF(efficiently)
DEF(Without)
DEF(enough)
DEF(data)
DEF(relocation)
DEF(sections)
DEF(are)
DEF(not)
DEF(sufficiently)
DEF(large)
DEF(and)
DEF(injected)
DEF(code)
DEF(wouldnt)
DEF(fit)
DEF(Said)
DEF(otherwise)
DEF(we)
DEF(need)
DEF(more)
DEF(words)
DEF(than)
DEF(up)
DEF(here)
DEF(so)
DEF(that)
DEF(relocations)
DEF(take)
DEF(significant)
DEF(bytes)
DEF(amounts)
DEF(which)
DEF(isnt)
DEF(exactly)
DEF(easily)
DEF(achieved)
DEF(like)
DEF(this)
DEF(Actually)
DEF(I)
DEF(must)
DEF(cheat)
DEF(by)
DEF(including)
DEF(these)
DEF(phrases)
DEF(several)
DEF(times)

#else
#pragma GCC visibility push(default)
#include <stdlib.h>
#include <stdio.h>

#define DEF(w) static const char str_ ## w[] = #w;
#include "test.c"
#undef DEF

const char *strings[] = {
#define DEF(w) str_ ## w,
#include "test.c"
#include "test.c"
#include "test.c"
};


const int hole[] = {
    42, 42, 42, 42
};

const char *strings2[] = {
#include "test.c"
#include "test.c"
#include "test.c"
#include "test.c"
#include "test.c"
#undef DEF
};

static int ret = 1;

int print_status() {
    fprintf(stderr, "%s\n", ret ? "FAIL" : "PASS");
    return ret;
}








__thread int foo;
__thread long long int bar[512];

void end_test() {
    static int count = 0;
    
    if (++count == 2)
        ret = 0;
}

void test() {
    int i = 0, j = 0;
#define DEF_(a,i,w) \
    if (a[i++] != str_ ## w) return;
#define DEF(w) DEF_(strings,i,w)
#include "test.c"
#include "test.c"
#include "test.c"
#undef DEF
#define DEF(w) DEF_(strings2,j,w)
#include "test.c"
#include "test.c"
#include "test.c"
#include "test.c"
#include "test.c"
#undef DEF
    if (i != sizeof(strings)/sizeof(strings[0]) &&
        j != sizeof(strings2)/sizeof(strings2[0]))
        fprintf(stderr, "WARNING: Test doesn't cover the whole array\n");
    end_test();
}

#pragma GCC visibility pop
#endif
