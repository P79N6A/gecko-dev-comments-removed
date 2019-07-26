













#include "primpl.h"

#if defined(_PR_PTHREADS) || defined(_PR_GLOBAL_THREADS_ONLY) || defined(QNX)

#else  

#ifdef IRIX
#include <unistd.h>
#include <bstring.h>
#endif

#include <string.h>
#include <sys/types.h>
#include <sys/time.h>

#define ZAP_SET(_to, _width)				      \
    PR_BEGIN_MACRO					      \
	memset(_to, 0,					      \
	       ((_width + 8*sizeof(int)-1) / (8*sizeof(int))) \
		* sizeof(int)				      \
	       );					      \
    PR_END_MACRO


static int _pr_xt_hack_fd = -1;
 
int PR_XGetXtHackFD(void)
{
    int fds[2];
 
    if (_pr_xt_hack_fd == -1) {
        if (!pipe(fds)) {
            _pr_xt_hack_fd = fds[0];
        }
    }
    return _pr_xt_hack_fd;
}

static int (*_pr_xt_hack_okayToReleaseXLock)(void) = 0;

void PR_SetXtHackOkayToReleaseXLockFn(int (*fn)(void))
{
    _pr_xt_hack_okayToReleaseXLock = fn; 
}












#if defined(HPUX9)
int select(size_t width, int *rl, int *wl, int *el, const struct timeval *tv)
#elif defined(AIX_RENAME_SELECT)
int wrap_select(unsigned long width, void *rl, void *wl, void *el,
        struct timeval *tv)
#elif defined(_PR_SELECT_CONST_TIMEVAL)
int select(int width, fd_set *rd, fd_set *wr, fd_set *ex,
        const struct timeval *tv)
#else
int select(int width, fd_set *rd, fd_set *wr, fd_set *ex, struct timeval *tv)
#endif
{
    int osfd;
    _PRUnixPollDesc *unixpds, *unixpd, *eunixpd;
    PRInt32 pdcnt;
    PRIntervalTime timeout;
    int retVal;
#if defined(HPUX9) || defined(AIX_RENAME_SELECT)
    fd_set *rd = (fd_set*) rl;
    fd_set *wr = (fd_set*) wl;
    fd_set *ex = (fd_set*) el;
#endif

#if 0
    



    if (tv != NULL && tv->tv_sec == 0 && tv->tv_usec == 0) {
#if defined(HPUX9) || defined(AIX_RENAME_SELECT)
        return _MD_SELECT(width, rl, wl, el, tv);
#else
        return _MD_SELECT(width, rd, wr, ex, tv);
#endif
    }
#endif

    if (!_pr_initialized) {
        _PR_ImplicitInitialization();
    }
		
#ifndef _PR_LOCAL_THREADS_ONLY
    if (_PR_IS_NATIVE_THREAD(_PR_MD_CURRENT_THREAD())) {
        return _MD_SELECT(width, rd, wr, ex, tv);	
    }
#endif

    if (width < 0 || width > FD_SETSIZE) {
        errno = EINVAL;
        return -1;
    }

    
    if (tv) {
        



        if (tv->tv_sec < 0 || tv->tv_sec > 100000000
                || tv->tv_usec < 0 || tv->tv_usec >= 1000000) {
            errno = EINVAL;
            return -1;
        }

        
        timeout = PR_MicrosecondsToInterval(1000000*tv->tv_sec + tv->tv_usec);
    } else {
        
        timeout = PR_INTERVAL_NO_TIMEOUT;
    }

    
    if ((!rd && !wr && !ex) || !width) {
        PR_Sleep(timeout);
        return 0;
    }

    









    unixpds = (_PRUnixPollDesc *) PR_CALLOC(width * sizeof(_PRUnixPollDesc));
    if (!unixpds) {
        errno = ENOMEM;
        return -1;
    }

    pdcnt = 0;
    unixpd = unixpds;
    for (osfd = 0; osfd < width; osfd++) {
        int in_flags = 0;
        if (rd && FD_ISSET(osfd, rd)) {
            in_flags |= _PR_UNIX_POLL_READ;
        }
        if (wr && FD_ISSET(osfd, wr)) {
            in_flags |= _PR_UNIX_POLL_WRITE;
        }
        if (ex && FD_ISSET(osfd, ex)) {
            in_flags |= _PR_UNIX_POLL_EXCEPT;
        }
        if (in_flags) {
            unixpd->osfd = osfd;
            unixpd->in_flags = in_flags;
            unixpd->out_flags = 0;
            unixpd++;
            pdcnt++;
        }
    }

    



   {
     int needToLockXAgain;
 
     needToLockXAgain = 0;
     if (rd && (_pr_xt_hack_fd != -1)
             && FD_ISSET(_pr_xt_hack_fd, rd) && PR_XIsLocked()
             && (!_pr_xt_hack_okayToReleaseXLock
             || _pr_xt_hack_okayToReleaseXLock())) {
         PR_XUnlock();
         needToLockXAgain = 1;
     }

    
    retVal = _PR_WaitForMultipleFDs(unixpds, pdcnt, timeout);

     if (needToLockXAgain) {
         PR_XLock();
     }
   }

    if (retVal > 0) {
        
        if (rd) ZAP_SET(rd, width);
        if (wr) ZAP_SET(wr, width);
        if (ex) ZAP_SET(ex, width);

        



        retVal = 0;  
        eunixpd = unixpds + pdcnt;
        for (unixpd = unixpds; unixpd < eunixpd; unixpd++) {
            if (unixpd->out_flags) {
                int nbits = 0;  

                if (unixpd->out_flags & _PR_UNIX_POLL_NVAL) {
                    errno = EBADF;
                    PR_LOG(_pr_io_lm, PR_LOG_ERROR,
                            ("select returns EBADF for %d", unixpd->osfd));
                    retVal = -1;
                    break;
                }
                






                if (rd && (unixpd->in_flags & _PR_UNIX_POLL_READ)
                        && (unixpd->out_flags & (_PR_UNIX_POLL_READ
                        | _PR_UNIX_POLL_ERR | _PR_UNIX_POLL_HUP))) {
                    FD_SET(unixpd->osfd, rd);
                    nbits++;
                }
                if (wr && (unixpd->in_flags & _PR_UNIX_POLL_WRITE)
                        && (unixpd->out_flags & (_PR_UNIX_POLL_WRITE
                        | _PR_UNIX_POLL_ERR))) {
                    FD_SET(unixpd->osfd, wr);
                    nbits++;
                }
                if (ex && (unixpd->in_flags & _PR_UNIX_POLL_WRITE)
                        && (unixpd->out_flags & PR_POLL_EXCEPT)) {
                    FD_SET(unixpd->osfd, ex);
                    nbits++;
                }
                PR_ASSERT(nbits > 0);
#if defined(HPUX) || defined(SOLARIS) || defined(OSF1) || defined(AIX)
                retVal += nbits;
#else 
                retVal += 1;
#endif
            }
        }
    }

    PR_ASSERT(tv || retVal != 0);
    PR_LOG(_pr_io_lm, PR_LOG_MIN, ("select returns %d", retVal));
    PR_DELETE(unixpds);

    return retVal;
}












#if defined(_PR_POLL_AVAILABLE) && !defined(LINUX)














#include <poll.h>

#if defined(AIX_RENAME_SELECT)
int wrap_poll(void *listptr, unsigned long nfds, long timeout)
#elif (defined(AIX) && !defined(AIX_RENAME_SELECT))
int poll(void *listptr, unsigned long nfds, long timeout)
#elif defined(OSF1) || (defined(HPUX) && !defined(HPUX9))
int poll(struct pollfd filedes[], unsigned int nfds, int timeout)
#elif defined(HPUX9)
int poll(struct pollfd filedes[], int nfds, int timeout)
#elif defined(NETBSD)
int poll(struct pollfd *filedes, nfds_t nfds, int timeout)
#elif defined(OPENBSD)
int poll(struct pollfd filedes[], nfds_t nfds, int timeout)
#elif defined(FREEBSD)
int poll(struct pollfd *filedes, unsigned nfds, int timeout)
#else
int poll(struct pollfd *filedes, unsigned long nfds, int timeout)
#endif
{
#ifdef AIX
    struct pollfd *filedes = (struct pollfd *) listptr;
#endif
    struct pollfd *pfd, *epfd;
    _PRUnixPollDesc *unixpds, *unixpd, *eunixpd;
    PRIntervalTime ticks;
    PRInt32 pdcnt;
    int ready;

    



    if (timeout == 0) {
#if defined(AIX)
        return _MD_POLL(listptr, nfds, timeout);
#else
        return _MD_POLL(filedes, nfds, timeout);
#endif
    }

    if (!_pr_initialized) {
        _PR_ImplicitInitialization();
    }

#ifndef _PR_LOCAL_THREADS_ONLY
    if (_PR_IS_NATIVE_THREAD(_PR_MD_CURRENT_THREAD())) {
    	return _MD_POLL(filedes, nfds, timeout);
    }
#endif

    
#ifdef AIX
    PR_ASSERT((nfds & 0xff00) == 0);
#endif

    if (timeout < 0 && timeout != -1) {
        errno = EINVAL;
        return -1;
    }

    
    if (timeout == -1) {
        ticks = PR_INTERVAL_NO_TIMEOUT;
    } else {
        ticks = PR_MillisecondsToInterval(timeout);
    }

    
    if (nfds == 0) {
        PR_Sleep(ticks);
        return 0;
    }

    unixpds = (_PRUnixPollDesc *)
            PR_MALLOC(nfds * sizeof(_PRUnixPollDesc));
    if (NULL == unixpds) {
        errno = EAGAIN;
        return -1;
    }

    pdcnt = 0;
    epfd = filedes + nfds;
    unixpd = unixpds;
    for (pfd = filedes; pfd < epfd; pfd++) {
        


        if (pfd->fd >= 0) {
            unixpd->osfd = pfd->fd;
#ifdef _PR_USE_POLL
            unixpd->in_flags = pfd->events;
#else
            











            unixpd->in_flags = 0;
            if (pfd->events & (POLLIN
#ifdef POLLRDNORM
                    | POLLRDNORM
#endif
                    )) {
                unixpd->in_flags |= _PR_UNIX_POLL_READ;
            }
            if (pfd->events & (POLLOUT
#ifdef POLLWRNORM
                    | POLLWRNORM
#endif
                    )) {
                unixpd->in_flags |= _PR_UNIX_POLL_WRITE;
            }
            if (pfd->events & (POLLPRI
#ifdef POLLRDBAND
                    | POLLRDBAND
#endif
                    )) {
                unixpd->in_flags |= PR_POLL_EXCEPT;
            }
#endif  
            unixpd->out_flags = 0;
            unixpd++;
            pdcnt++;
        }
    }

    ready = _PR_WaitForMultipleFDs(unixpds, pdcnt, ticks);
    if (-1 == ready) {
        if (PR_GetError() == PR_PENDING_INTERRUPT_ERROR) {
            errno = EINTR;  
        } else {
            errno = PR_GetOSError();
        }
    }
    if (ready <= 0) {
        goto done;
    }

    



    unixpd = unixpds;
    for (pfd = filedes; pfd < epfd; pfd++) {
        pfd->revents = 0;
        if (pfd->fd >= 0) {
#ifdef _PR_USE_POLL
            pfd->revents = unixpd->out_flags;
#else
            if (0 != unixpd->out_flags) {
                if (unixpd->out_flags & _PR_UNIX_POLL_READ) {
                    if (pfd->events & POLLIN) {
                        pfd->revents |= POLLIN;
                    }
#ifdef POLLRDNORM
                    if (pfd->events & POLLRDNORM) {
                        pfd->revents |= POLLRDNORM;
                    }
#endif
                }
                if (unixpd->out_flags & _PR_UNIX_POLL_WRITE) {
                    if (pfd->events & POLLOUT) {
                        pfd->revents |= POLLOUT;
                    }
#ifdef POLLWRNORM
                    if (pfd->events & POLLWRNORM) {
                        pfd->revents |= POLLWRNORM;
                    }
#endif
                }
                if (unixpd->out_flags & _PR_UNIX_POLL_EXCEPT) {
                    if (pfd->events & POLLPRI) {
                        pfd->revents |= POLLPRI;
                    }
#ifdef POLLRDBAND
                    if (pfd->events & POLLRDBAND) {
                        pfd->revents |= POLLRDBAND;
                    }
#endif
                }
                if (unixpd->out_flags & _PR_UNIX_POLL_ERR) {
                    pfd->revents |= POLLERR;
                }
                if (unixpd->out_flags & _PR_UNIX_POLL_NVAL) {
                    pfd->revents |= POLLNVAL;
                }
                if (unixpd->out_flags & _PR_UNIX_POLL_HUP) {
                    pfd->revents |= POLLHUP;
                }
            }
#endif  
            unixpd++;
        }
    }

done:
    PR_DELETE(unixpds);
    return ready;
}

#endif  

#endif  



