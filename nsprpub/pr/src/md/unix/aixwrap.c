














#ifndef AIX_RENAME_SELECT
#error aixwrap.c should only be used on AIX 3.2 or 4.1
#else

#include <sys/select.h>
#include <sys/poll.h>

int _MD_SELECT(int width, fd_set *r, fd_set *w, fd_set *e, struct timeval *t)
{
    return select(width, r, w, e, t);
}

int _MD_POLL(void *listptr, unsigned long nfds, long timeout)
{
    return poll(listptr, nfds, timeout);
}

#endif 
