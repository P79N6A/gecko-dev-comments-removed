










































#include <unistd.h>













void
mcount()
{
    register unsigned int caller;
    unsigned int buf[1];

    
    asm("movl 4(%%esp),%0"
        : "=r"(caller));

    buf[0] = caller;
    write(STDOUT_FILENO, buf, sizeof buf[0]);
}
