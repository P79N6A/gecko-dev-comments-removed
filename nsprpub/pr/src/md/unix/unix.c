




































#include "primpl.h"

#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/utsname.h>

#ifdef _PR_POLL_AVAILABLE
#include <poll.h>
#endif


#if defined(NCR) || defined(UNIXWARE) || defined(NEC) || defined(SNI) \
        || defined(SONY)
#include <sys/filio.h>
#endif

#if defined(NTO)
#include <sys/statvfs.h>
#endif





#if defined(HAVE_SOCKLEN_T) \
    || (defined(__GLIBC__) && __GLIBC__ >= 2)
#define _PRSockLen_t socklen_t
#elif defined(IRIX) || defined(HPUX) || defined(OSF1) || defined(SOLARIS) \
    || defined(AIX4_1) || defined(LINUX) || defined(SONY) \
    || defined(BSDI) || defined(SCO) || defined(NEC) || defined(SNI) \
    || defined(SUNOS4) || defined(NCR) || defined(DARWIN) \
    || defined(NEXTSTEP) || defined(QNX)
#define _PRSockLen_t int
#elif (defined(AIX) && !defined(AIX4_1)) || defined(FREEBSD) \
    || defined(NETBSD) || defined(OPENBSD) || defined(UNIXWARE) \
    || defined(DGUX) || defined(VMS) || defined(NTO) || defined(RISCOS)
#define _PRSockLen_t size_t
#else
#error "Cannot determine architecture"
#endif





static PRLock *_pr_rename_lock = NULL;
static PRMonitor *_pr_Xfe_mon = NULL;

static PRInt64 minus_one;

sigset_t timer_set;

#if !defined(_PR_PTHREADS)

static sigset_t empty_set;

#ifdef SOLARIS
#include <sys/file.h>
#include <sys/filio.h>
#endif

#ifndef PIPE_BUF
#define PIPE_BUF 512
#endif




int _nspr_noclock = 1;

#ifdef IRIX
extern PRInt32 _nspr_terminate_on_error;
#endif








int _pr_md_pipefd[2] = { -1, -1 };
static char _pr_md_pipebuf[PIPE_BUF];
static PRInt32 local_io_wait(PRInt32 osfd, PRInt32 wait_flag,
							PRIntervalTime timeout);

_PRInterruptTable _pr_interruptTable[] = {
    { 
        "clock", _PR_MISSED_CLOCK, _PR_ClockInterrupt,     },
    { 
        0     }
};

void _MD_unix_init_running_cpu(_PRCPU *cpu)
{
    PR_INIT_CLIST(&(cpu->md.md_unix.ioQ));
    cpu->md.md_unix.ioq_max_osfd = -1;
    cpu->md.md_unix.ioq_timeout = PR_INTERVAL_NO_TIMEOUT;
}

PRStatus _MD_open_dir(_MDDir *d, const char *name)
{
int err;

    d->d = opendir(name);
    if (!d->d) {
        err = _MD_ERRNO();
        _PR_MD_MAP_OPENDIR_ERROR(err);
        return PR_FAILURE;
    }
    return PR_SUCCESS;
}

PRInt32 _MD_close_dir(_MDDir *d)
{
int rv = 0, err;

    if (d->d) {
        rv = closedir(d->d);
        if (rv == -1) {
                err = _MD_ERRNO();
                _PR_MD_MAP_CLOSEDIR_ERROR(err);
        }
    }
    return rv;
}

char * _MD_read_dir(_MDDir *d, PRIntn flags)
{
struct dirent *de;
int err;

    for (;;) {
        



        _MD_ERRNO() = 0;
        de = readdir(d->d);
        if (!de) {
            err = _MD_ERRNO();
            _PR_MD_MAP_READDIR_ERROR(err);
            return 0;
        }        
        if ((flags & PR_SKIP_DOT) &&
            (de->d_name[0] == '.') && (de->d_name[1] == 0))
            continue;
        if ((flags & PR_SKIP_DOT_DOT) &&
            (de->d_name[0] == '.') && (de->d_name[1] == '.') &&
            (de->d_name[2] == 0))
            continue;
        if ((flags & PR_SKIP_HIDDEN) && (de->d_name[0] == '.'))
            continue;
        break;
    }
    return de->d_name;
}

PRInt32 _MD_delete(const char *name)
{
PRInt32 rv, err;
#ifdef UNIXWARE
    sigset_t set, oset;
#endif

#ifdef UNIXWARE
    sigfillset(&set);
    sigprocmask(SIG_SETMASK, &set, &oset);
#endif
    rv = unlink(name);
#ifdef UNIXWARE
    sigprocmask(SIG_SETMASK, &oset, NULL);
#endif
    if (rv == -1) {
            err = _MD_ERRNO();
            _PR_MD_MAP_UNLINK_ERROR(err);
    }
    return(rv);
}

PRInt32 _MD_rename(const char *from, const char *to)
{
    PRInt32 rv = -1, err;

    





    if (NULL != _pr_rename_lock)
        PR_Lock(_pr_rename_lock);
    if (0 == access(to, F_OK))
        PR_SetError(PR_FILE_EXISTS_ERROR, 0);
    else
    {
        rv = rename(from, to);
        if (rv < 0) {
            err = _MD_ERRNO();
            _PR_MD_MAP_RENAME_ERROR(err);
        }
    }
    if (NULL != _pr_rename_lock)
        PR_Unlock(_pr_rename_lock);
    return rv;
}

PRInt32 _MD_access(const char *name, PRAccessHow how)
{
PRInt32 rv, err;
int amode;

    switch (how) {
        case PR_ACCESS_WRITE_OK:
            amode = W_OK;
            break;
        case PR_ACCESS_READ_OK:
            amode = R_OK;
            break;
        case PR_ACCESS_EXISTS:
            amode = F_OK;
            break;
        default:
            PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
            rv = -1;
            goto done;
    }
    rv = access(name, amode);

    if (rv < 0) {
        err = _MD_ERRNO();
        _PR_MD_MAP_ACCESS_ERROR(err);
    }

done:
    return(rv);
}

PRInt32 _MD_mkdir(const char *name, PRIntn mode)
{
int rv, err;

    



    if (NULL !=_pr_rename_lock)
        PR_Lock(_pr_rename_lock);
    rv = mkdir(name, mode);
    if (rv < 0) {
        err = _MD_ERRNO();
        _PR_MD_MAP_MKDIR_ERROR(err);
    }
    if (NULL !=_pr_rename_lock)
        PR_Unlock(_pr_rename_lock);
    return rv;
}

PRInt32 _MD_rmdir(const char *name)
{
int rv, err;

    rv = rmdir(name);
    if (rv == -1) {
            err = _MD_ERRNO();
            _PR_MD_MAP_RMDIR_ERROR(err);
    }
    return rv;
}

PRInt32 _MD_read(PRFileDesc *fd, void *buf, PRInt32 amount)
{
PRThread *me = _PR_MD_CURRENT_THREAD();
PRInt32 rv, err;
#ifndef _PR_USE_POLL
fd_set rd;
#else
struct pollfd pfd;
#endif 
PRInt32 osfd = fd->secret->md.osfd;

#ifndef _PR_USE_POLL
    FD_ZERO(&rd);
    FD_SET(osfd, &rd);
#else
    pfd.fd = osfd;
    pfd.events = POLLIN;
#endif 
    while ((rv = read(osfd,buf,amount)) == -1) {
        err = _MD_ERRNO();
        if ((err == EAGAIN) || (err == EWOULDBLOCK)) {
            if (fd->secret->nonblocking) {
                break;
            }
            if (!_PR_IS_NATIVE_THREAD(me)) {
                if ((rv = local_io_wait(osfd, _PR_UNIX_POLL_READ,
										PR_INTERVAL_NO_TIMEOUT)) < 0)
					goto done;								
            } else {
#ifndef _PR_USE_POLL
                while ((rv = _MD_SELECT(osfd + 1, &rd, NULL, NULL, NULL))
                        == -1 && (err = _MD_ERRNO()) == EINTR) {
                    
                }
#else 
                while ((rv = _MD_POLL(&pfd, 1, -1))
                        == -1 && (err = _MD_ERRNO()) == EINTR) {
                    
                }
#endif 
                if (rv == -1) {
                    break;
                }
            }
            if (_PR_PENDING_INTERRUPT(me))
                break;
        } else if ((err == EINTR) && (!_PR_PENDING_INTERRUPT(me))){
            continue;
        } else {
            break;
        }
    }
    if (rv < 0) {
        if (_PR_PENDING_INTERRUPT(me)) {
            me->flags &= ~_PR_INTERRUPT;
            PR_SetError(PR_PENDING_INTERRUPT_ERROR, 0);
        } else {
            _PR_MD_MAP_READ_ERROR(err);
        }
    }
done:
    return(rv);
}

PRInt32 _MD_write(PRFileDesc *fd, const void *buf, PRInt32 amount)
{
PRThread *me = _PR_MD_CURRENT_THREAD();
PRInt32 rv, err;
#ifndef _PR_USE_POLL
fd_set wd;
#else
struct pollfd pfd;
#endif 
PRInt32 osfd = fd->secret->md.osfd;

#ifndef _PR_USE_POLL
    FD_ZERO(&wd);
    FD_SET(osfd, &wd);
#else
    pfd.fd = osfd;
    pfd.events = POLLOUT;
#endif 
    while ((rv = write(osfd,buf,amount)) == -1) {
        err = _MD_ERRNO();
        if ((err == EAGAIN) || (err == EWOULDBLOCK)) {
            if (fd->secret->nonblocking) {
                break;
            }
            if (!_PR_IS_NATIVE_THREAD(me)) {
                if ((rv = local_io_wait(osfd, _PR_UNIX_POLL_WRITE,
										PR_INTERVAL_NO_TIMEOUT)) < 0)
                    goto done;
            } else {
#ifndef _PR_USE_POLL
                while ((rv = _MD_SELECT(osfd + 1, NULL, &wd, NULL, NULL))
                        == -1 && (err = _MD_ERRNO()) == EINTR) {
                    
                }
#else 
                while ((rv = _MD_POLL(&pfd, 1, -1))
                        == -1 && (err = _MD_ERRNO()) == EINTR) {
                    
                }
#endif 
                if (rv == -1) {
                    break;
                }
            }
            if (_PR_PENDING_INTERRUPT(me))
                break;
        } else if ((err == EINTR) && (!_PR_PENDING_INTERRUPT(me))){
            continue;
        } else {
            break;
        }
    }
    if (rv < 0) {
        if (_PR_PENDING_INTERRUPT(me)) {
            me->flags &= ~_PR_INTERRUPT;
            PR_SetError(PR_PENDING_INTERRUPT_ERROR, 0);
        } else {
            _PR_MD_MAP_WRITE_ERROR(err);
        }
    }
done:
    return(rv);
}

PRInt32 _MD_fsync(PRFileDesc *fd)
{
PRInt32 rv, err;

    rv = fsync(fd->secret->md.osfd);
    if (rv == -1) {
        err = _MD_ERRNO();
        _PR_MD_MAP_FSYNC_ERROR(err);
    }
    return(rv);
}

PRInt32 _MD_close(PRInt32 osfd)
{
PRInt32 rv, err;

    rv = close(osfd);
    if (rv == -1) {
        err = _MD_ERRNO();
        _PR_MD_MAP_CLOSE_ERROR(err);
    }
    return(rv);
}

PRInt32 _MD_socket(PRInt32 domain, PRInt32 type, PRInt32 proto)
{
    PRInt32 osfd, err;

    osfd = socket(domain, type, proto);

    if (osfd == -1) {
        err = _MD_ERRNO();
        _PR_MD_MAP_SOCKET_ERROR(err);
        return(osfd);
    }

    return(osfd);
}

PRInt32 _MD_socketavailable(PRFileDesc *fd)
{
    PRInt32 result;

    if (ioctl(fd->secret->md.osfd, FIONREAD, &result) < 0) {
        _PR_MD_MAP_SOCKETAVAILABLE_ERROR(_MD_ERRNO());
        return -1;
    }
    return result;
}

PRInt64 _MD_socketavailable64(PRFileDesc *fd)
{
    PRInt64 result;
    LL_I2L(result, _MD_socketavailable(fd));
    return result;
}  

#define READ_FD        1
#define WRITE_FD    2










#ifndef _PR_USE_POLL

static PRInt32 socket_io_wait(PRInt32 osfd, PRInt32 fd_type,
    PRIntervalTime timeout)
{
    PRInt32 rv = -1;
    struct timeval tv;
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PRIntervalTime epoch, now, elapsed, remaining;
    PRBool wait_for_remaining;
    PRInt32 syserror;
    fd_set rd_wr;

    switch (timeout) {
        case PR_INTERVAL_NO_WAIT:
            PR_SetError(PR_IO_TIMEOUT_ERROR, 0);
            break;
        case PR_INTERVAL_NO_TIMEOUT:
            



            tv.tv_sec = _PR_INTERRUPT_CHECK_INTERVAL_SECS;
            tv.tv_usec = 0;
            FD_ZERO(&rd_wr);
            do {
                FD_SET(osfd, &rd_wr);
                if (fd_type == READ_FD)
                    rv = _MD_SELECT(osfd + 1, &rd_wr, NULL, NULL, &tv);
                else
                    rv = _MD_SELECT(osfd + 1, NULL, &rd_wr, NULL, &tv);
                if (rv == -1 && (syserror = _MD_ERRNO()) != EINTR) {
                    _PR_MD_MAP_SELECT_ERROR(syserror);
                    break;
                }
                if (_PR_PENDING_INTERRUPT(me)) {
                    me->flags &= ~_PR_INTERRUPT;
                    PR_SetError(PR_PENDING_INTERRUPT_ERROR, 0);
                    rv = -1;
                    break;
                }
            } while (rv == 0 || (rv == -1 && syserror == EINTR));
            break;
        default:
            now = epoch = PR_IntervalNow();
            remaining = timeout;
            FD_ZERO(&rd_wr);
            do {
                





                wait_for_remaining = PR_TRUE;
                tv.tv_sec = PR_IntervalToSeconds(remaining);
                if (tv.tv_sec > _PR_INTERRUPT_CHECK_INTERVAL_SECS) {
                    wait_for_remaining = PR_FALSE;
                    tv.tv_sec = _PR_INTERRUPT_CHECK_INTERVAL_SECS;
                    tv.tv_usec = 0;
                } else {
                    tv.tv_usec = PR_IntervalToMicroseconds(
                        remaining -
                        PR_SecondsToInterval(tv.tv_sec));
                }
                FD_SET(osfd, &rd_wr);
                if (fd_type == READ_FD)
                    rv = _MD_SELECT(osfd + 1, &rd_wr, NULL, NULL, &tv);
                else
                    rv = _MD_SELECT(osfd + 1, NULL, &rd_wr, NULL, &tv);
                


                if (rv == -1 && (syserror = _MD_ERRNO()) != EINTR) {
                    _PR_MD_MAP_SELECT_ERROR(syserror);
                    break;
                }
                if (_PR_PENDING_INTERRUPT(me)) {
                    me->flags &= ~_PR_INTERRUPT;
                    PR_SetError(PR_PENDING_INTERRUPT_ERROR, 0);
                    rv = -1;
                    break;
                }
                



                if (rv == 0 || (rv == -1 && syserror == EINTR)) {
                    




                    if (rv == 0) {
                        if (wait_for_remaining) {
                            now += remaining;
                        } else {
                            now += PR_SecondsToInterval(tv.tv_sec)
                                + PR_MicrosecondsToInterval(tv.tv_usec);
                        }
                    } else {
                        now = PR_IntervalNow();
                    }
                    elapsed = (PRIntervalTime) (now - epoch);
                    if (elapsed >= timeout) {
                        PR_SetError(PR_IO_TIMEOUT_ERROR, 0);
                        rv = -1;
                        break;
                    } else {
                        remaining = timeout - elapsed;
                    }
                }
            } while (rv == 0 || (rv == -1 && syserror == EINTR));
            break;
    }
    return(rv);
}

#else 

static PRInt32 socket_io_wait(PRInt32 osfd, PRInt32 fd_type,
    PRIntervalTime timeout)
{
    PRInt32 rv = -1;
    int msecs;
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PRIntervalTime epoch, now, elapsed, remaining;
    PRBool wait_for_remaining;
    PRInt32 syserror;
    struct pollfd pfd;

    switch (timeout) {
        case PR_INTERVAL_NO_WAIT:
            PR_SetError(PR_IO_TIMEOUT_ERROR, 0);
            break;
        case PR_INTERVAL_NO_TIMEOUT:
            



            msecs = _PR_INTERRUPT_CHECK_INTERVAL_SECS * 1000;
            pfd.fd = osfd;
            if (fd_type == READ_FD) {
                pfd.events = POLLIN;
            } else {
                pfd.events = POLLOUT;
            }
            do {
                rv = _MD_POLL(&pfd, 1, msecs);
                if (rv == -1 && (syserror = _MD_ERRNO()) != EINTR) {
                    _PR_MD_MAP_POLL_ERROR(syserror);
                    break;
                }
				


                if ((rv == 1) && (pfd.revents & (POLLHUP | POLLNVAL))) {
					rv = -1;
                    _PR_MD_MAP_POLL_REVENTS_ERROR(pfd.revents);
                    break;
                }
                if (_PR_PENDING_INTERRUPT(me)) {
                    me->flags &= ~_PR_INTERRUPT;
                    PR_SetError(PR_PENDING_INTERRUPT_ERROR, 0);
                    rv = -1;
                    break;
                }
            } while (rv == 0 || (rv == -1 && syserror == EINTR));
            break;
        default:
            now = epoch = PR_IntervalNow();
            remaining = timeout;
            pfd.fd = osfd;
            if (fd_type == READ_FD) {
                pfd.events = POLLIN;
            } else {
                pfd.events = POLLOUT;
            }
            do {
                





                wait_for_remaining = PR_TRUE;
                msecs = PR_IntervalToMilliseconds(remaining);
                if (msecs > _PR_INTERRUPT_CHECK_INTERVAL_SECS * 1000) {
                    wait_for_remaining = PR_FALSE;
                    msecs = _PR_INTERRUPT_CHECK_INTERVAL_SECS * 1000;
                }
                rv = _MD_POLL(&pfd, 1, msecs);
                


                if (rv == -1 && (syserror = _MD_ERRNO()) != EINTR) {
                    _PR_MD_MAP_POLL_ERROR(syserror);
                    break;
                }
                if (_PR_PENDING_INTERRUPT(me)) {
                    me->flags &= ~_PR_INTERRUPT;
                    PR_SetError(PR_PENDING_INTERRUPT_ERROR, 0);
                    rv = -1;
                    break;
                }
				


                if ((rv == 1) && (pfd.revents & (POLLHUP | POLLNVAL))) {
					rv = -1;
                    _PR_MD_MAP_POLL_REVENTS_ERROR(pfd.revents);
                    break;
                }
                



                if (rv == 0 || (rv == -1 && syserror == EINTR)) {
                    




                    if (rv == 0) {
                        if (wait_for_remaining) {
                            now += remaining;
                        } else {
                            now += PR_MillisecondsToInterval(msecs);
                        }
                    } else {
                        now = PR_IntervalNow();
                    }
                    elapsed = (PRIntervalTime) (now - epoch);
                    if (elapsed >= timeout) {
                        PR_SetError(PR_IO_TIMEOUT_ERROR, 0);
                        rv = -1;
                        break;
                    } else {
                        remaining = timeout - elapsed;
                    }
                }
            } while (rv == 0 || (rv == -1 && syserror == EINTR));
            break;
    }
    return(rv);
}

#endif 

static PRInt32 local_io_wait(
    PRInt32 osfd,
    PRInt32 wait_flag,
    PRIntervalTime timeout)
{
    _PRUnixPollDesc pd;
    PRInt32 rv;

    PR_LOG(_pr_io_lm, PR_LOG_MIN,
       ("waiting to %s on osfd=%d",
        (wait_flag == _PR_UNIX_POLL_READ) ? "read" : "write",
        osfd));

    if (timeout == PR_INTERVAL_NO_WAIT) return 0;

    pd.osfd = osfd;
    pd.in_flags = wait_flag;
    pd.out_flags = 0;

    rv = _PR_WaitForMultipleFDs(&pd, 1, timeout);

    if (rv == 0) {
        PR_SetError(PR_IO_TIMEOUT_ERROR, 0);
        rv = -1;
    }
    return rv;
}


PRInt32 _MD_recv(PRFileDesc *fd, void *buf, PRInt32 amount,
                                PRInt32 flags, PRIntervalTime timeout)
{
    PRInt32 osfd = fd->secret->md.osfd;
    PRInt32 rv, err;
    PRThread *me = _PR_MD_CURRENT_THREAD();






#if defined(UNIXWARE) || defined(SOLARIS) || defined(NCR)
    while ((rv = read(osfd,buf,amount)) == -1) {
#else
    while ((rv = recv(osfd,buf,amount,flags)) == -1) {
#endif
        err = _MD_ERRNO();
        if ((err == EAGAIN) || (err == EWOULDBLOCK)) {
            if (fd->secret->nonblocking) {
                break;
            }
            if (!_PR_IS_NATIVE_THREAD(me)) {
				if ((rv = local_io_wait(osfd,_PR_UNIX_POLL_READ,timeout)) < 0)
					goto done;
            } else {
                if ((rv = socket_io_wait(osfd, READ_FD, timeout)) < 0)
                    goto done;
            }
        } else if ((err == EINTR) && (!_PR_PENDING_INTERRUPT(me))){
            continue;
        } else {
            break;
        }
    }
    if (rv < 0) {
        _PR_MD_MAP_RECV_ERROR(err);
    }
done:
    return(rv);
}

PRInt32 _MD_recvfrom(PRFileDesc *fd, void *buf, PRInt32 amount,
                        PRIntn flags, PRNetAddr *addr, PRUint32 *addrlen,
                        PRIntervalTime timeout)
{
    PRInt32 osfd = fd->secret->md.osfd;
    PRInt32 rv, err;
    PRThread *me = _PR_MD_CURRENT_THREAD();

    while ((*addrlen = PR_NETADDR_SIZE(addr)),
                ((rv = recvfrom(osfd, buf, amount, flags,
                        (struct sockaddr *) addr, (_PRSockLen_t *)addrlen)) == -1)) {
        err = _MD_ERRNO();
        if ((err == EAGAIN) || (err == EWOULDBLOCK)) {
            if (fd->secret->nonblocking) {
                break;
            }
            if (!_PR_IS_NATIVE_THREAD(me)) {
                if ((rv = local_io_wait(osfd, _PR_UNIX_POLL_READ, timeout)) < 0)
                    goto done;
            } else {
                if ((rv = socket_io_wait(osfd, READ_FD, timeout)) < 0)
                    goto done;
            }
        } else if ((err == EINTR) && (!_PR_PENDING_INTERRUPT(me))){
            continue;
        } else {
            break;
        }
    }
    if (rv < 0) {
        _PR_MD_MAP_RECVFROM_ERROR(err);
    }
done:
#ifdef _PR_HAVE_SOCKADDR_LEN
    if (rv != -1) {
        
        if (addr) {
            addr->raw.family = ((struct sockaddr *) addr)->sa_family;
        }
    }
#endif 
    return(rv);
}

PRInt32 _MD_send(PRFileDesc *fd, const void *buf, PRInt32 amount,
                            PRInt32 flags, PRIntervalTime timeout)
{
    PRInt32 osfd = fd->secret->md.osfd;
    PRInt32 rv, err;
    PRThread *me = _PR_MD_CURRENT_THREAD();
#if defined(SOLARIS)
	PRInt32 tmp_amount = amount;
#endif

    




#if defined(SOLARIS)
    PR_ASSERT(0 == flags);
    while ((rv = write(osfd,buf,tmp_amount)) == -1) {
#else
    while ((rv = send(osfd,buf,amount,flags)) == -1) {
#endif
        err = _MD_ERRNO();
        if ((err == EAGAIN) || (err == EWOULDBLOCK))    {
            if (fd->secret->nonblocking) {
                break;
            }
            if (!_PR_IS_NATIVE_THREAD(me)) {
                if ((rv = local_io_wait(osfd, _PR_UNIX_POLL_WRITE, timeout)) < 0)
                    goto done;
            } else {
                if ((rv = socket_io_wait(osfd, WRITE_FD, timeout))< 0)
                    goto done;
            }
        } else if ((err == EINTR) && (!_PR_PENDING_INTERRUPT(me))){
            continue;
        } else {
#if defined(SOLARIS)
			




			if (err == ERANGE) {
				if (tmp_amount > 1) {
					tmp_amount = tmp_amount/2;	
					continue;
				}
			}
#endif
            break;
        }
    }
        




    if ((!fd->secret->nonblocking) && (rv > 0) && (rv < amount)
            && (timeout != PR_INTERVAL_NO_WAIT)) {
        if (_PR_IS_NATIVE_THREAD(me)) {
			if (socket_io_wait(osfd, WRITE_FD, timeout)< 0) {
				rv = -1;
				goto done;
			}
        } else {
			if (local_io_wait(osfd, _PR_UNIX_POLL_WRITE, timeout) < 0) {
				rv = -1;
				goto done;
			}
        }
    }
    if (rv < 0) {
        _PR_MD_MAP_SEND_ERROR(err);
    }
done:
    return(rv);
}

PRInt32 _MD_sendto(
    PRFileDesc *fd, const void *buf, PRInt32 amount, PRIntn flags,
    const PRNetAddr *addr, PRUint32 addrlen, PRIntervalTime timeout)
{
    PRInt32 osfd = fd->secret->md.osfd;
    PRInt32 rv, err;
    PRThread *me = _PR_MD_CURRENT_THREAD();
#ifdef _PR_HAVE_SOCKADDR_LEN
    PRNetAddr addrCopy;

    addrCopy = *addr;
    ((struct sockaddr *) &addrCopy)->sa_len = addrlen;
    ((struct sockaddr *) &addrCopy)->sa_family = addr->raw.family;

    while ((rv = sendto(osfd, buf, amount, flags,
            (struct sockaddr *) &addrCopy, addrlen)) == -1) {
#else
    while ((rv = sendto(osfd, buf, amount, flags,
            (struct sockaddr *) addr, addrlen)) == -1) {
#endif
        err = _MD_ERRNO();
        if ((err == EAGAIN) || (err == EWOULDBLOCK))    {
            if (fd->secret->nonblocking) {
                break;
            }
            if (!_PR_IS_NATIVE_THREAD(me)) {
				if ((rv = local_io_wait(osfd, _PR_UNIX_POLL_WRITE, timeout)) < 0)
					goto done;
            } else {
                if ((rv = socket_io_wait(osfd, WRITE_FD, timeout))< 0)
                    goto done;
            }
        } else if ((err == EINTR) && (!_PR_PENDING_INTERRUPT(me))){
            continue;
        } else {
            break;
        }
    }
    if (rv < 0) {
        _PR_MD_MAP_SENDTO_ERROR(err);
    }
done:
    return(rv);
}

PRInt32 _MD_writev(
    PRFileDesc *fd, const PRIOVec *iov,
    PRInt32 iov_size, PRIntervalTime timeout)
{
    PRInt32 rv, err;
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PRInt32 index, amount = 0;
    PRInt32 osfd = fd->secret->md.osfd;

    






    if (!fd->secret->nonblocking) {
        for (index=0; index<iov_size; index++) {
            amount += iov[index].iov_len;
        }
    }

    while ((rv = writev(osfd, (const struct iovec*)iov, iov_size)) == -1) {
        err = _MD_ERRNO();
        if ((err == EAGAIN) || (err == EWOULDBLOCK))    {
            if (fd->secret->nonblocking) {
                break;
            }
            if (!_PR_IS_NATIVE_THREAD(me)) {
				if ((rv = local_io_wait(osfd, _PR_UNIX_POLL_WRITE, timeout)) < 0)
					goto done;
            } else {
                if ((rv = socket_io_wait(osfd, WRITE_FD, timeout))<0)
                    goto done;
            }
        } else if ((err == EINTR) && (!_PR_PENDING_INTERRUPT(me))){
            continue;
        } else {
            break;
        }
    }
    




    if ((!fd->secret->nonblocking) && (rv > 0) && (rv < amount)
            && (timeout != PR_INTERVAL_NO_WAIT)) {
        if (_PR_IS_NATIVE_THREAD(me)) {
            if (socket_io_wait(osfd, WRITE_FD, timeout) < 0) {
				rv = -1;
                goto done;
			}
        } else {
			if (local_io_wait(osfd, _PR_UNIX_POLL_WRITE, timeout) < 0) {
				rv = -1;
				goto done;
			}
        }
    }
    if (rv < 0) {
        _PR_MD_MAP_WRITEV_ERROR(err);
    }
done:
    return(rv);
}

PRInt32 _MD_accept(PRFileDesc *fd, PRNetAddr *addr,
                            PRUint32 *addrlen, PRIntervalTime timeout)
{
    PRInt32 osfd = fd->secret->md.osfd;
    PRInt32 rv, err;
    PRThread *me = _PR_MD_CURRENT_THREAD();

    while ((rv = accept(osfd, (struct sockaddr *) addr,
                                        (_PRSockLen_t *)addrlen)) == -1) {
        err = _MD_ERRNO();
        if ((err == EAGAIN) || (err == EWOULDBLOCK) || (err == ECONNABORTED)) {
            if (fd->secret->nonblocking) {
                break;
            }
            if (!_PR_IS_NATIVE_THREAD(me)) {
				if ((rv = local_io_wait(osfd, _PR_UNIX_POLL_READ, timeout)) < 0)
					goto done;
            } else {
                if ((rv = socket_io_wait(osfd, READ_FD, timeout)) < 0)
                    goto done;
            }
        } else if ((err == EINTR) && (!_PR_PENDING_INTERRUPT(me))){
            continue;
        } else {
            break;
        }
    }
    if (rv < 0) {
        _PR_MD_MAP_ACCEPT_ERROR(err);
    }
done:
#ifdef _PR_HAVE_SOCKADDR_LEN
    if (rv != -1) {
        
        if (addr) {
            addr->raw.family = ((struct sockaddr *) addr)->sa_family;
        }
    }
#endif 
    return(rv);
}

extern int _connect (int s, const struct sockaddr *name, int namelen);
PRInt32 _MD_connect(
    PRFileDesc *fd, const PRNetAddr *addr, PRUint32 addrlen, PRIntervalTime timeout)
{
    PRInt32 rv, err;
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PRInt32 osfd = fd->secret->md.osfd;
#ifdef IRIX
extern PRInt32 _MD_irix_connect(
        PRInt32 osfd, const PRNetAddr *addr, PRInt32 addrlen, PRIntervalTime timeout);
#endif
#ifdef _PR_HAVE_SOCKADDR_LEN
    PRNetAddr addrCopy;

    addrCopy = *addr;
    ((struct sockaddr *) &addrCopy)->sa_len = addrlen;
    ((struct sockaddr *) &addrCopy)->sa_family = addr->raw.family;
#endif

    











retry:
#ifdef IRIX
    if ((rv = _MD_irix_connect(osfd, addr, addrlen, timeout)) == -1) {
#else
#ifdef _PR_HAVE_SOCKADDR_LEN
    if ((rv = connect(osfd, (struct sockaddr *)&addrCopy, addrlen)) == -1) {
#else
    if ((rv = connect(osfd, (struct sockaddr *)addr, addrlen)) == -1) {
#endif
#endif
        err = _MD_ERRNO();

        if (err == EINTR) {
            if (_PR_PENDING_INTERRUPT(me)) {
                me->flags &= ~_PR_INTERRUPT;
                PR_SetError( PR_PENDING_INTERRUPT_ERROR, 0);
                return -1;
            }
            goto retry;
        }

        if (!fd->secret->nonblocking && (err == EINPROGRESS)) {
            if (!_PR_IS_NATIVE_THREAD(me)) {

				if ((rv = local_io_wait(osfd, _PR_UNIX_POLL_WRITE, timeout)) < 0)
                    return -1;
            } else {
                



                rv = socket_io_wait(osfd, WRITE_FD, timeout);
                if (rv == -1) {
                    return -1;
                }
            }

            PR_ASSERT(rv == 1);
            if (_PR_PENDING_INTERRUPT(me)) {
                me->flags &= ~_PR_INTERRUPT;
                PR_SetError( PR_PENDING_INTERRUPT_ERROR, 0);
                return -1;
            }
            err = _MD_unix_get_nonblocking_connect_error(osfd);
            if (err != 0) {
                _PR_MD_MAP_CONNECT_ERROR(err);
                return -1;
            }
            return 0;
        }

        _PR_MD_MAP_CONNECT_ERROR(err);
    }

    return rv;
}  

PRInt32 _MD_bind(PRFileDesc *fd, const PRNetAddr *addr, PRUint32 addrlen)
{
    PRInt32 rv, err;
#ifdef _PR_HAVE_SOCKADDR_LEN
    PRNetAddr addrCopy;

    addrCopy = *addr;
    ((struct sockaddr *) &addrCopy)->sa_len = addrlen;
    ((struct sockaddr *) &addrCopy)->sa_family = addr->raw.family;
    rv = bind(fd->secret->md.osfd, (struct sockaddr *) &addrCopy, (int )addrlen);
#else
    rv = bind(fd->secret->md.osfd, (struct sockaddr *) addr, (int )addrlen);
#endif
    if (rv < 0) {
        err = _MD_ERRNO();
        _PR_MD_MAP_BIND_ERROR(err);
    }
    return(rv);
}

PRInt32 _MD_listen(PRFileDesc *fd, PRIntn backlog)
{
    PRInt32 rv, err;

    rv = listen(fd->secret->md.osfd, backlog);
    if (rv < 0) {
        err = _MD_ERRNO();
        _PR_MD_MAP_LISTEN_ERROR(err);
    }
    return(rv);
}

PRInt32 _MD_shutdown(PRFileDesc *fd, PRIntn how)
{
    PRInt32 rv, err;

    rv = shutdown(fd->secret->md.osfd, how);
    if (rv < 0) {
        err = _MD_ERRNO();
        _PR_MD_MAP_SHUTDOWN_ERROR(err);
    }
    return(rv);
}

PRInt32 _MD_socketpair(int af, int type, int flags,
                                                        PRInt32 *osfd)
{
    PRInt32 rv, err;

    rv = socketpair(af, type, flags, osfd);
    if (rv < 0) {
        err = _MD_ERRNO();
        _PR_MD_MAP_SOCKETPAIR_ERROR(err);
    }
    return rv;
}

PRStatus _MD_getsockname(PRFileDesc *fd, PRNetAddr *addr,
                                                PRUint32 *addrlen)
{
    PRInt32 rv, err;

    rv = getsockname(fd->secret->md.osfd,
            (struct sockaddr *) addr, (_PRSockLen_t *)addrlen);
#ifdef _PR_HAVE_SOCKADDR_LEN
    if (rv == 0) {
        
        if (addr) {
            addr->raw.family = ((struct sockaddr *) addr)->sa_family;
        }
    }
#endif 
    if (rv < 0) {
        err = _MD_ERRNO();
        _PR_MD_MAP_GETSOCKNAME_ERROR(err);
    }
    return rv==0?PR_SUCCESS:PR_FAILURE;
}

PRStatus _MD_getpeername(PRFileDesc *fd, PRNetAddr *addr,
                                        PRUint32 *addrlen)
{
    PRInt32 rv, err;

    rv = getpeername(fd->secret->md.osfd,
            (struct sockaddr *) addr, (_PRSockLen_t *)addrlen);
#ifdef _PR_HAVE_SOCKADDR_LEN
    if (rv == 0) {
        
        if (addr) {
            addr->raw.family = ((struct sockaddr *) addr)->sa_family;
        }
    }
#endif 
    if (rv < 0) {
        err = _MD_ERRNO();
        _PR_MD_MAP_GETPEERNAME_ERROR(err);
    }
    return rv==0?PR_SUCCESS:PR_FAILURE;
}

PRStatus _MD_getsockopt(PRFileDesc *fd, PRInt32 level,
                        PRInt32 optname, char* optval, PRInt32* optlen)
{
    PRInt32 rv, err;

    rv = getsockopt(fd->secret->md.osfd, level, optname, optval, (_PRSockLen_t *)optlen);
    if (rv < 0) {
        err = _MD_ERRNO();
        _PR_MD_MAP_GETSOCKOPT_ERROR(err);
    }
    return rv==0?PR_SUCCESS:PR_FAILURE;
}

PRStatus _MD_setsockopt(PRFileDesc *fd, PRInt32 level,   
                    PRInt32 optname, const char* optval, PRInt32 optlen)
{
    PRInt32 rv, err;

    rv = setsockopt(fd->secret->md.osfd, level, optname, optval, optlen);
    if (rv < 0) {
        err = _MD_ERRNO();
        _PR_MD_MAP_SETSOCKOPT_ERROR(err);
    }
    return rv==0?PR_SUCCESS:PR_FAILURE;
}

PRStatus _MD_set_fd_inheritable(PRFileDesc *fd, PRBool inheritable)
{
    int rv;

    rv = fcntl(fd->secret->md.osfd, F_SETFD, inheritable ? 0 : FD_CLOEXEC);
    if (-1 == rv) {
        PR_SetError(PR_UNKNOWN_ERROR, _MD_ERRNO());
        return PR_FAILURE;
    }
    return PR_SUCCESS;
}

void _MD_init_fd_inheritable(PRFileDesc *fd, PRBool imported)
{
    if (imported) {
        fd->secret->inheritable = _PR_TRI_UNKNOWN;
    } else {
        
#ifdef DEBUG
        {
            int flags = fcntl(fd->secret->md.osfd, F_GETFD, 0);
            PR_ASSERT(0 == flags);
        }
#endif
        fd->secret->inheritable = _PR_TRI_TRUE;
    }
}


#if !defined(_PR_USE_POLL)





static void FindBadFDs(void)
{
    PRCList *q;
    PRThread *me = _MD_CURRENT_THREAD();

    PR_ASSERT(!_PR_IS_NATIVE_THREAD(me));
    q = (_PR_IOQ(me->cpu)).next;
    _PR_IOQ_MAX_OSFD(me->cpu) = -1;
    _PR_IOQ_TIMEOUT(me->cpu) = PR_INTERVAL_NO_TIMEOUT;
    while (q != &_PR_IOQ(me->cpu)) {
        PRPollQueue *pq = _PR_POLLQUEUE_PTR(q);
        PRBool notify = PR_FALSE;
        _PRUnixPollDesc *pds = pq->pds;
        _PRUnixPollDesc *epds = pds + pq->npds;
        PRInt32 pq_max_osfd = -1;

        q = q->next;
        for (; pds < epds; pds++) {
            PRInt32 osfd = pds->osfd;
            pds->out_flags = 0;
            PR_ASSERT(osfd >= 0 || pds->in_flags == 0);
            if (pds->in_flags == 0) {
                continue;  
            }
            if (fcntl(osfd, F_GETFL, 0) == -1) {
                
                PR_LOG(_pr_io_lm, PR_LOG_MAX,
                    ("file descriptor %d is bad", osfd));
                pds->out_flags = _PR_UNIX_POLL_NVAL;
                notify = PR_TRUE;
            }
            if (osfd > pq_max_osfd) {
                pq_max_osfd = osfd;
            }
        }

        if (notify) {
            PRIntn pri;
            PR_REMOVE_LINK(&pq->links);
            pq->on_ioq = PR_FALSE;

            




            pds = pq->pds;
            for (; pds < epds; pds++) {
                PRInt32 osfd = pds->osfd;
                PRInt16 in_flags = pds->in_flags;
                PR_ASSERT(osfd >= 0 || in_flags == 0);
                if (in_flags & _PR_UNIX_POLL_READ) {
                    if (--(_PR_FD_READ_CNT(me->cpu))[osfd] == 0)
                        FD_CLR(osfd, &_PR_FD_READ_SET(me->cpu));
                }
                if (in_flags & _PR_UNIX_POLL_WRITE) {
                    if (--(_PR_FD_WRITE_CNT(me->cpu))[osfd] == 0)
                        FD_CLR(osfd, &_PR_FD_WRITE_SET(me->cpu));
                }
                if (in_flags & _PR_UNIX_POLL_EXCEPT) {
                    if (--(_PR_FD_EXCEPTION_CNT(me->cpu))[osfd] == 0)
                        FD_CLR(osfd, &_PR_FD_EXCEPTION_SET(me->cpu));
                }
            }

            _PR_THREAD_LOCK(pq->thr);
            if (pq->thr->flags & (_PR_ON_PAUSEQ|_PR_ON_SLEEPQ)) {
                _PRCPU *cpu = pq->thr->cpu;
                _PR_SLEEPQ_LOCK(pq->thr->cpu);
                _PR_DEL_SLEEPQ(pq->thr, PR_TRUE);
                _PR_SLEEPQ_UNLOCK(pq->thr->cpu);

				if (pq->thr->flags & _PR_SUSPENDING) {
				    




				    pq->thr->state = _PR_SUSPENDED;
				    _PR_MISCQ_LOCK(pq->thr->cpu);
				    _PR_ADD_SUSPENDQ(pq->thr, pq->thr->cpu);
				    _PR_MISCQ_UNLOCK(pq->thr->cpu);
				} else {
				    pri = pq->thr->priority;
				    pq->thr->state = _PR_RUNNABLE;

				    _PR_RUNQ_LOCK(cpu);
				    _PR_ADD_RUNQ(pq->thr, cpu, pri);
				    _PR_RUNQ_UNLOCK(cpu);
				}
            }
            _PR_THREAD_UNLOCK(pq->thr);
        } else {
            if (pq->timeout < _PR_IOQ_TIMEOUT(me->cpu))
                _PR_IOQ_TIMEOUT(me->cpu) = pq->timeout;
            if (_PR_IOQ_MAX_OSFD(me->cpu) < pq_max_osfd)
                _PR_IOQ_MAX_OSFD(me->cpu) = pq_max_osfd;
        }
    }
    if (_PR_IS_NATIVE_THREAD_SUPPORTED()) {
        if (_PR_IOQ_MAX_OSFD(me->cpu) < _pr_md_pipefd[0])
            _PR_IOQ_MAX_OSFD(me->cpu) = _pr_md_pipefd[0];
    }
}
#endif  














void _MD_PauseCPU(PRIntervalTime ticks)
{
    PRThread *me = _MD_CURRENT_THREAD();
#ifdef _PR_USE_POLL
    int timeout;
    struct pollfd *pollfds;    
    struct pollfd *pollfdPtr;    
    unsigned long npollfds;     
    unsigned long pollfds_size;
    int nfd;                    
#else
    struct timeval timeout, *tvp;
    fd_set r, w, e;
    fd_set *rp, *wp, *ep;
    PRInt32 max_osfd, nfd;
#endif  
    PRInt32 rv;
    PRCList *q;
    PRUint32 min_timeout;
    sigset_t oldset;
#ifdef IRIX
extern sigset_t ints_off;
#endif

    PR_ASSERT(_PR_MD_GET_INTSOFF() != 0);

    _PR_MD_IOQ_LOCK();

#ifdef _PR_USE_POLL
    

    
    npollfds = _PR_IOQ_OSFD_CNT(me->cpu);
    PR_ASSERT(npollfds >= 0);

    



    if (_PR_IS_NATIVE_THREAD_SUPPORTED()) {
        npollfds++;
#ifdef	IRIX
		




		if (me->cpu->id == 0) {
        	npollfds++;
		}
#endif
	}

    


    if (npollfds > _PR_IOQ_POLLFDS_SIZE(me->cpu)) {
        if (_PR_IOQ_POLLFDS(me->cpu) != NULL)
            PR_DELETE(_PR_IOQ_POLLFDS(me->cpu));
        pollfds_size =  PR_MAX(_PR_IOQ_MIN_POLLFDS_SIZE(me->cpu), npollfds);
        pollfds = (struct pollfd *) PR_MALLOC(pollfds_size * sizeof(struct pollfd));
        _PR_IOQ_POLLFDS(me->cpu) = pollfds;
        _PR_IOQ_POLLFDS_SIZE(me->cpu) = pollfds_size;
    } else {
        pollfds = _PR_IOQ_POLLFDS(me->cpu);
    }
    pollfdPtr = pollfds;

    



    if (_PR_IS_NATIVE_THREAD_SUPPORTED()) {
        pollfdPtr->fd = _pr_md_pipefd[0];
        pollfdPtr->events = POLLIN;
        pollfdPtr++;
#ifdef	IRIX
		


		if (me->cpu->id == 0) {
			pollfdPtr->fd = _pr_irix_primoridal_cpu_fd[0];
			pollfdPtr->events = POLLIN;
			pollfdPtr++;
		}
#endif
    }

    min_timeout = PR_INTERVAL_NO_TIMEOUT;
    for (q = _PR_IOQ(me->cpu).next; q != &_PR_IOQ(me->cpu); q = q->next) {
        PRPollQueue *pq = _PR_POLLQUEUE_PTR(q);
        _PRUnixPollDesc *pds = pq->pds;
        _PRUnixPollDesc *epds = pds + pq->npds;

        if (pq->timeout < min_timeout) {
            min_timeout = pq->timeout;
        }
        for (; pds < epds; pds++, pollfdPtr++) {
            



            PR_ASSERT(pollfdPtr < pollfds + npollfds);
            pollfdPtr->fd = pds->osfd;
            
            pollfdPtr->events = pds->in_flags;
        }
    }
    _PR_IOQ_TIMEOUT(me->cpu) = min_timeout;
#else
    


    r = _PR_FD_READ_SET(me->cpu);
    w = _PR_FD_WRITE_SET(me->cpu);
    e = _PR_FD_EXCEPTION_SET(me->cpu);

    rp = &r;
    wp = &w;
    ep = &e;

    max_osfd = _PR_IOQ_MAX_OSFD(me->cpu) + 1;
    min_timeout = _PR_IOQ_TIMEOUT(me->cpu);
#endif  
    




    q = _PR_SLEEPQ(me->cpu).next;

    if (q != &_PR_SLEEPQ(me->cpu)) {
        PRThread *t = _PR_THREAD_PTR(q);

        if (t->sleep < min_timeout) {
            min_timeout = t->sleep;
        }
    }
    if (min_timeout > ticks) {
        min_timeout = ticks;
    }

#ifdef _PR_USE_POLL
    if (min_timeout == PR_INTERVAL_NO_TIMEOUT)
        timeout = -1;
    else
        timeout = PR_IntervalToMilliseconds(min_timeout);
#else
    if (min_timeout == PR_INTERVAL_NO_TIMEOUT) {
        tvp = NULL;
    } else {
        timeout.tv_sec = PR_IntervalToSeconds(min_timeout);
        timeout.tv_usec = PR_IntervalToMicroseconds(min_timeout)
            % PR_USEC_PER_SEC;
        tvp = &timeout;
    }
#endif  

    _PR_MD_IOQ_UNLOCK();
    _MD_CHECK_FOR_EXIT();
    


#ifndef _PR_NO_CLOCK_TIMER
    




#ifdef IRIX
    




    if ((!_nspr_noclock) || (_nspr_terminate_on_error))
#else
        if (!_nspr_noclock)
#endif    
#ifdef IRIX
    sigprocmask(SIG_BLOCK, &ints_off, &oldset);
#else
    PR_ASSERT(sigismember(&timer_set, SIGALRM));
    sigprocmask(SIG_BLOCK, &timer_set, &oldset);
#endif    
#endif  

#ifndef _PR_USE_POLL
    PR_ASSERT(FD_ISSET(_pr_md_pipefd[0],rp));
    nfd = _MD_SELECT(max_osfd, rp, wp, ep, tvp);
#else
    nfd = _MD_POLL(pollfds, npollfds, timeout);
#endif  

#ifndef _PR_NO_CLOCK_TIMER
#ifdef IRIX
    if ((!_nspr_noclock) || (_nspr_terminate_on_error))
#else
        if (!_nspr_noclock)
#endif    
    sigprocmask(SIG_SETMASK, &oldset, 0);
#endif  

    _MD_CHECK_FOR_EXIT();

#ifdef IRIX
	_PR_MD_primordial_cpu();
#endif

    _PR_MD_IOQ_LOCK();
    


#ifdef _PR_USE_POLL
    if (nfd > 0) {
        pollfdPtr = pollfds;
        if (_PR_IS_NATIVE_THREAD_SUPPORTED()) {
            



            PR_ASSERT(pollfds[0].fd == _pr_md_pipefd[0]);
            if ((pollfds[0].revents & POLLIN) && (nfd == 1)) {
                



                while ((rv = read(_pr_md_pipefd[0], _pr_md_pipebuf,
                    PIPE_BUF)) == PIPE_BUF){
                }
                PR_ASSERT((rv > 0) || ((rv == -1) && (errno == EAGAIN)));
            }
            pollfdPtr++;
#ifdef	IRIX
			



			if (me->cpu->id == 0) {
            	PR_ASSERT(pollfds[1].fd == _pr_irix_primoridal_cpu_fd[0]);
				if (pollfdPtr->revents & POLLIN) {
					if (_pr_irix_process_exit) {
						


						prctl(PR_SETEXITSIG, SIGKILL);
						_exit(_pr_irix_process_exit_code);
					} else {
						while ((rv = read(_pr_irix_primoridal_cpu_fd[0],
							_pr_md_pipebuf, PIPE_BUF)) == PIPE_BUF) {
						}
						PR_ASSERT(rv > 0);
					}
				}
				pollfdPtr++;
			}
#endif
        }
        for (q = _PR_IOQ(me->cpu).next; q != &_PR_IOQ(me->cpu); q = q->next) {
            PRPollQueue *pq = _PR_POLLQUEUE_PTR(q);
            PRBool notify = PR_FALSE;
            _PRUnixPollDesc *pds = pq->pds;
            _PRUnixPollDesc *epds = pds + pq->npds;

            for (; pds < epds; pds++, pollfdPtr++) {
                



                PR_ASSERT(pollfdPtr < pollfds + npollfds);
                





                PR_ASSERT(pollfdPtr->fd == pds->osfd);
                pds->out_flags = pollfdPtr->revents;
                
                if (pds->osfd >= 0 && pds->out_flags) {
                    notify = PR_TRUE;
                }
            }
            if (notify) {
                PRIntn pri;
                PRThread *thred;

                PR_REMOVE_LINK(&pq->links);
                pq->on_ioq = PR_FALSE;

                thred = pq->thr;
                _PR_THREAD_LOCK(thred);
                if (pq->thr->flags & (_PR_ON_PAUSEQ|_PR_ON_SLEEPQ)) {
                    _PRCPU *cpu = pq->thr->cpu;
                    _PR_SLEEPQ_LOCK(pq->thr->cpu);
                    _PR_DEL_SLEEPQ(pq->thr, PR_TRUE);
                    _PR_SLEEPQ_UNLOCK(pq->thr->cpu);

					if (pq->thr->flags & _PR_SUSPENDING) {
					    




					    pq->thr->state = _PR_SUSPENDED;
					    _PR_MISCQ_LOCK(pq->thr->cpu);
					    _PR_ADD_SUSPENDQ(pq->thr, pq->thr->cpu);
					    _PR_MISCQ_UNLOCK(pq->thr->cpu);
					} else {
						pri = pq->thr->priority;
						pq->thr->state = _PR_RUNNABLE;

						_PR_RUNQ_LOCK(cpu);
						_PR_ADD_RUNQ(pq->thr, cpu, pri);
						_PR_RUNQ_UNLOCK(cpu);
						if (_pr_md_idle_cpus > 1)
							_PR_MD_WAKEUP_WAITER(thred);
					}
                }
                _PR_THREAD_UNLOCK(thred);
                _PR_IOQ_OSFD_CNT(me->cpu) -= pq->npds;
                PR_ASSERT(_PR_IOQ_OSFD_CNT(me->cpu) >= 0);
            }
        }
    } else if (nfd == -1) {
        PR_LOG(_pr_io_lm, PR_LOG_MAX, ("poll() failed with errno %d", errno));
    }

#else
    if (nfd > 0) {
        q = _PR_IOQ(me->cpu).next;
        _PR_IOQ_MAX_OSFD(me->cpu) = -1;
        _PR_IOQ_TIMEOUT(me->cpu) = PR_INTERVAL_NO_TIMEOUT;
        while (q != &_PR_IOQ(me->cpu)) {
            PRPollQueue *pq = _PR_POLLQUEUE_PTR(q);
            PRBool notify = PR_FALSE;
            _PRUnixPollDesc *pds = pq->pds;
            _PRUnixPollDesc *epds = pds + pq->npds;
            PRInt32 pq_max_osfd = -1;

            q = q->next;
            for (; pds < epds; pds++) {
                PRInt32 osfd = pds->osfd;
                PRInt16 in_flags = pds->in_flags;
                PRInt16 out_flags = 0;
                PR_ASSERT(osfd >= 0 || in_flags == 0);
                if ((in_flags & _PR_UNIX_POLL_READ) && FD_ISSET(osfd, rp)) {
                    out_flags |= _PR_UNIX_POLL_READ;
                }
                if ((in_flags & _PR_UNIX_POLL_WRITE) && FD_ISSET(osfd, wp)) {
                    out_flags |= _PR_UNIX_POLL_WRITE;
                }
                if ((in_flags & _PR_UNIX_POLL_EXCEPT) && FD_ISSET(osfd, ep)) {
                    out_flags |= _PR_UNIX_POLL_EXCEPT;
                }
                pds->out_flags = out_flags;
                if (out_flags) {
                    notify = PR_TRUE;
                }
                if (osfd > pq_max_osfd) {
                    pq_max_osfd = osfd;
                }
            }
            if (notify == PR_TRUE) {
                PRIntn pri;
                PRThread *thred;

                PR_REMOVE_LINK(&pq->links);
                pq->on_ioq = PR_FALSE;

                




                pds = pq->pds;
                for (; pds < epds; pds++) {
                    PRInt32 osfd = pds->osfd;
                    PRInt16 in_flags = pds->in_flags;
                    PR_ASSERT(osfd >= 0 || in_flags == 0);
                    if (in_flags & _PR_UNIX_POLL_READ) {
                        if (--(_PR_FD_READ_CNT(me->cpu))[osfd] == 0)
                            FD_CLR(osfd, &_PR_FD_READ_SET(me->cpu));
                    }
                    if (in_flags & _PR_UNIX_POLL_WRITE) {
                        if (--(_PR_FD_WRITE_CNT(me->cpu))[osfd] == 0)
                            FD_CLR(osfd, &_PR_FD_WRITE_SET(me->cpu));
                    }
                    if (in_flags & _PR_UNIX_POLL_EXCEPT) {
                        if (--(_PR_FD_EXCEPTION_CNT(me->cpu))[osfd] == 0)
                            FD_CLR(osfd, &_PR_FD_EXCEPTION_SET(me->cpu));
                    }
                }

                




                 thred = pq->thr;
                _PR_THREAD_LOCK(thred);
                if (pq->thr->flags & (_PR_ON_PAUSEQ|_PR_ON_SLEEPQ)) {
                    _PRCPU *cpu = thred->cpu;
                    _PR_SLEEPQ_LOCK(pq->thr->cpu);
                    _PR_DEL_SLEEPQ(pq->thr, PR_TRUE);
                    _PR_SLEEPQ_UNLOCK(pq->thr->cpu);

					if (pq->thr->flags & _PR_SUSPENDING) {
					    




					    pq->thr->state = _PR_SUSPENDED;
					    _PR_MISCQ_LOCK(pq->thr->cpu);
					    _PR_ADD_SUSPENDQ(pq->thr, pq->thr->cpu);
					    _PR_MISCQ_UNLOCK(pq->thr->cpu);
					} else {
						pri = pq->thr->priority;
						pq->thr->state = _PR_RUNNABLE;

						pq->thr->cpu = cpu;
						_PR_RUNQ_LOCK(cpu);
						_PR_ADD_RUNQ(pq->thr, cpu, pri);
						_PR_RUNQ_UNLOCK(cpu);
						if (_pr_md_idle_cpus > 1)
							_PR_MD_WAKEUP_WAITER(thred);
					}
                }
                _PR_THREAD_UNLOCK(thred);
            } else {
                if (pq->timeout < _PR_IOQ_TIMEOUT(me->cpu))
                    _PR_IOQ_TIMEOUT(me->cpu) = pq->timeout;
                if (_PR_IOQ_MAX_OSFD(me->cpu) < pq_max_osfd)
                    _PR_IOQ_MAX_OSFD(me->cpu) = pq_max_osfd;
            }
        }
        if (_PR_IS_NATIVE_THREAD_SUPPORTED()) {
            if ((FD_ISSET(_pr_md_pipefd[0], rp)) && (nfd == 1)) {
                



                while ((rv =
                    read(_pr_md_pipefd[0], _pr_md_pipebuf, PIPE_BUF))
                    == PIPE_BUF){
                }
                PR_ASSERT((rv > 0) ||
                    ((rv == -1) && (errno == EAGAIN)));
            }
            if (_PR_IOQ_MAX_OSFD(me->cpu) < _pr_md_pipefd[0])
                _PR_IOQ_MAX_OSFD(me->cpu) = _pr_md_pipefd[0];
#ifdef	IRIX
			if ((me->cpu->id == 0) && 
						(FD_ISSET(_pr_irix_primoridal_cpu_fd[0], rp))) {
				if (_pr_irix_process_exit) {
					


					prctl(PR_SETEXITSIG, SIGKILL);
					_exit(_pr_irix_process_exit_code);
				} else {
						while ((rv = read(_pr_irix_primoridal_cpu_fd[0],
							_pr_md_pipebuf, PIPE_BUF)) == PIPE_BUF) {
						}
						PR_ASSERT(rv > 0);
				}
			}
			if (me->cpu->id == 0) {
				if (_PR_IOQ_MAX_OSFD(me->cpu) < _pr_irix_primoridal_cpu_fd[0])
					_PR_IOQ_MAX_OSFD(me->cpu) = _pr_irix_primoridal_cpu_fd[0];
			}
#endif
        }
    } else if (nfd < 0) {
        if (errno == EBADF) {
            FindBadFDs();
        } else {
            PR_LOG(_pr_io_lm, PR_LOG_MAX, ("select() failed with errno %d",
                errno));
        }
    } else {
        PR_ASSERT(nfd == 0);
        


        q = _PR_IOQ(me->cpu).next;
        _PR_IOQ_MAX_OSFD(me->cpu) = -1;
        _PR_IOQ_TIMEOUT(me->cpu) = PR_INTERVAL_NO_TIMEOUT;
        while (q != &_PR_IOQ(me->cpu)) {
            PRPollQueue *pq = _PR_POLLQUEUE_PTR(q);
            _PRUnixPollDesc *pds = pq->pds;
            _PRUnixPollDesc *epds = pds + pq->npds;
            PRInt32 pq_max_osfd = -1;

            q = q->next;
            for (; pds < epds; pds++) {
                if (pds->osfd > pq_max_osfd) {
                    pq_max_osfd = pds->osfd;
                }
            }
            if (pq->timeout < _PR_IOQ_TIMEOUT(me->cpu))
                _PR_IOQ_TIMEOUT(me->cpu) = pq->timeout;
            if (_PR_IOQ_MAX_OSFD(me->cpu) < pq_max_osfd)
                _PR_IOQ_MAX_OSFD(me->cpu) = pq_max_osfd;
        }
        if (_PR_IS_NATIVE_THREAD_SUPPORTED()) {
            if (_PR_IOQ_MAX_OSFD(me->cpu) < _pr_md_pipefd[0])
                _PR_IOQ_MAX_OSFD(me->cpu) = _pr_md_pipefd[0];
        }
    }
#endif  
    _PR_MD_IOQ_UNLOCK();
}

void _MD_Wakeup_CPUs()
{
    PRInt32 rv, data;

    data = 0;
    rv = write(_pr_md_pipefd[1], &data, 1);

    while ((rv < 0) && (errno == EAGAIN)) {
        


        while ((rv =
            read(_pr_md_pipefd[0], _pr_md_pipebuf, PIPE_BUF))
            == PIPE_BUF) {
        }
        PR_ASSERT((rv > 0) ||
            ((rv == -1) && (errno == EAGAIN)));
        rv = write(_pr_md_pipefd[1], &data, 1);
    }
}


void _MD_InitCPUS()
{
    PRInt32 rv, flags;
    PRThread *me = _MD_CURRENT_THREAD();

    rv = pipe(_pr_md_pipefd);
    PR_ASSERT(rv == 0);
    _PR_IOQ_MAX_OSFD(me->cpu) = _pr_md_pipefd[0];
#ifndef _PR_USE_POLL
    FD_SET(_pr_md_pipefd[0], &_PR_FD_READ_SET(me->cpu));
#endif

    flags = fcntl(_pr_md_pipefd[0], F_GETFL, 0);
    fcntl(_pr_md_pipefd[0], F_SETFL, flags | O_NONBLOCK);
    flags = fcntl(_pr_md_pipefd[1], F_GETFL, 0);
    fcntl(_pr_md_pipefd[1], F_SETFL, flags | O_NONBLOCK);
}




static void ClockInterruptHandler()
{
    int olderrno;
    PRUintn pri;
    _PRCPU *cpu = _PR_MD_CURRENT_CPU();
    PRThread *me = _MD_CURRENT_THREAD();

#ifdef SOLARIS
    if (!me || _PR_IS_NATIVE_THREAD(me)) {
        _pr_primordialCPU->u.missed[_pr_primordialCPU->where] |= _PR_MISSED_CLOCK;
        return;
    }
#endif

    if (_PR_MD_GET_INTSOFF() != 0) {
        cpu->u.missed[cpu->where] |= _PR_MISSED_CLOCK;
        return;
    }
    _PR_MD_SET_INTSOFF(1);

    olderrno = errno;
    _PR_ClockInterrupt();
    errno = olderrno;

    



    pri = me->priority;
    if ((cpu->u.missed[3] || (_PR_RUNQREADYMASK(me->cpu) >> pri))) {
#ifdef _PR_NO_PREEMPT
        cpu->resched = PR_TRUE;
        if (pr_interruptSwitchHook) {
            (*pr_interruptSwitchHook)(pr_interruptSwitchHookArg);
        }
#else 
        




        sigprocmask(SIG_SETMASK, &empty_set, 0);
        PR_LOG(_pr_sched_lm, PR_LOG_MIN, ("clock caused context switch"));

        if(!(me->flags & _PR_IDLE_THREAD)) {
            _PR_THREAD_LOCK(me);
            me->state = _PR_RUNNABLE;
            me->cpu = cpu;
            _PR_RUNQ_LOCK(cpu);
            _PR_ADD_RUNQ(me, cpu, pri);
            _PR_RUNQ_UNLOCK(cpu);
            _PR_THREAD_UNLOCK(me);
        } else
            me->state = _PR_RUNNABLE;
        _MD_SWITCH_CONTEXT(me);
        PR_LOG(_pr_sched_lm, PR_LOG_MIN, ("clock back from context switch"));
#endif 
    }
    




    _PR_MD_SET_INTSOFF(0);
}







#ifdef HPUX9
static void HPUX9_ClockInterruptHandler(
    int sig,
    int code,
    struct sigcontext *scp)
{
    ClockInterruptHandler();
    scp->sc_syscall_action = SIG_RESTART;
}
#endif 


#define MSEC_PER_TICK    50


void _MD_StartInterrupts()
{
    char *eval;

    if ((eval = getenv("NSPR_NOCLOCK")) != NULL) {
        if (atoi(eval) == 0)
            _nspr_noclock = 0;
        else
            _nspr_noclock = 1;
    }

#ifndef _PR_NO_CLOCK_TIMER
    if (!_nspr_noclock) {
        _MD_EnableClockInterrupts();
    }
#endif
}

void _MD_StopInterrupts()
{
    sigprocmask(SIG_BLOCK, &timer_set, 0);
}

void _MD_EnableClockInterrupts()
{
    struct itimerval itval;
    extern PRUintn _pr_numCPU;
#ifdef HPUX9
    struct sigvec vec;

    vec.sv_handler = (void (*)()) HPUX9_ClockInterruptHandler;
    vec.sv_mask = 0;
    vec.sv_flags = 0;
    sigvector(SIGALRM, &vec, 0);
#else
    struct sigaction vtact;

    vtact.sa_handler = (void (*)()) ClockInterruptHandler;
    sigemptyset(&vtact.sa_mask);
    vtact.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &vtact, 0);
#endif 

    PR_ASSERT(_pr_numCPU == 1);
	itval.it_interval.tv_sec = 0;
	itval.it_interval.tv_usec = MSEC_PER_TICK * PR_USEC_PER_MSEC;
	itval.it_value = itval.it_interval;
	setitimer(ITIMER_REAL, &itval, 0);
}

void _MD_DisableClockInterrupts()
{
    struct itimerval itval;
    extern PRUintn _pr_numCPU;

    PR_ASSERT(_pr_numCPU == 1);
	itval.it_interval.tv_sec = 0;
	itval.it_interval.tv_usec = 0;
	itval.it_value = itval.it_interval;
	setitimer(ITIMER_REAL, &itval, 0);
}

void _MD_BlockClockInterrupts()
{
    sigprocmask(SIG_BLOCK, &timer_set, 0);
}

void _MD_UnblockClockInterrupts()
{
    sigprocmask(SIG_UNBLOCK, &timer_set, 0);
}

void _MD_MakeNonblock(PRFileDesc *fd)
{
    PRInt32 osfd = fd->secret->md.osfd;
    int flags;

    if (osfd <= 2) {
        
        return;
    }
    flags = fcntl(osfd, F_GETFL, 0);

    





#ifdef SUNOS4
    fcntl(osfd, F_SETFL, flags | FNDELAY);
#else
    fcntl(osfd, F_SETFL, flags | O_NONBLOCK);
#endif
    }

PRInt32 _MD_open(const char *name, PRIntn flags, PRIntn mode)
{
    PRInt32 osflags;
    PRInt32 rv, err;

    if (flags & PR_RDWR) {
        osflags = O_RDWR;
    } else if (flags & PR_WRONLY) {
        osflags = O_WRONLY;
    } else {
        osflags = O_RDONLY;
    }

    if (flags & PR_EXCL)
        osflags |= O_EXCL;
    if (flags & PR_APPEND)
        osflags |= O_APPEND;
    if (flags & PR_TRUNCATE)
        osflags |= O_TRUNC;
    if (flags & PR_SYNC) {
#if defined(O_SYNC)
        osflags |= O_SYNC;
#elif defined(O_FSYNC)
        osflags |= O_FSYNC;
#else
#error "Neither O_SYNC nor O_FSYNC is defined on this platform"
#endif
    }

    



    if (flags & PR_CREATE_FILE)
    {
        osflags |= O_CREAT;
        if (NULL !=_pr_rename_lock)
            PR_Lock(_pr_rename_lock);
    }

    rv = _md_iovector._open64(name, osflags, mode);

    if (rv < 0) {
        err = _MD_ERRNO();
        _PR_MD_MAP_OPEN_ERROR(err);
    }

    if ((flags & PR_CREATE_FILE) && (NULL !=_pr_rename_lock))
        PR_Unlock(_pr_rename_lock);
    return rv;
}

PRIntervalTime intr_timeout_ticks;

#if defined(SOLARIS) || defined(IRIX)
static void sigsegvhandler() {
    fprintf(stderr,"Received SIGSEGV\n");
    fflush(stderr);
    pause();
}

static void sigaborthandler() {
    fprintf(stderr,"Received SIGABRT\n");
    fflush(stderr);
    pause();
}

static void sigbushandler() {
    fprintf(stderr,"Received SIGBUS\n");
    fflush(stderr);
    pause();
}
#endif 

#endif  

void _MD_query_fd_inheritable(PRFileDesc *fd)
{
    int flags;

    PR_ASSERT(_PR_TRI_UNKNOWN == fd->secret->inheritable);
    flags = fcntl(fd->secret->md.osfd, F_GETFD, 0);
    PR_ASSERT(-1 != flags);
    fd->secret->inheritable = (flags & FD_CLOEXEC) ?
        _PR_TRI_FALSE : _PR_TRI_TRUE;
}

PROffset32 _MD_lseek(PRFileDesc *fd, PROffset32 offset, PRSeekWhence whence)
{
    PROffset32 rv, where;

    switch (whence) {
        case PR_SEEK_SET:
            where = SEEK_SET;
            break;
        case PR_SEEK_CUR:
            where = SEEK_CUR;
            break;
        case PR_SEEK_END:
            where = SEEK_END;
            break;
        default:
            PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
            rv = -1;
            goto done;
    }
    rv = lseek(fd->secret->md.osfd,offset,where);
    if (rv == -1)
    {
        PRInt32 syserr = _MD_ERRNO();
        _PR_MD_MAP_LSEEK_ERROR(syserr);
    }
done:
    return(rv);
}

PROffset64 _MD_lseek64(PRFileDesc *fd, PROffset64 offset, PRSeekWhence whence)
{
    PRInt32 where;
    PROffset64 rv;

    switch (whence)
    {
        case PR_SEEK_SET:
            where = SEEK_SET;
            break;
        case PR_SEEK_CUR:
            where = SEEK_CUR;
            break;
        case PR_SEEK_END:
            where = SEEK_END;
            break;
        default:
            PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
            rv = minus_one;
            goto done;
    }
    rv = _md_iovector._lseek64(fd->secret->md.osfd, offset, where);
    if (LL_EQ(rv, minus_one))
    {
        PRInt32 syserr = _MD_ERRNO();
        _PR_MD_MAP_LSEEK_ERROR(syserr);
    }
done:
    return rv;
}  











#if defined(_PR_STAT_HAS_ST_ATIM)




static void _MD_set_fileinfo_times(
    const struct stat *sb,
    PRFileInfo *info)
{
    PRInt64 us, s2us;

    LL_I2L(s2us, PR_USEC_PER_SEC);
    LL_I2L(info->modifyTime, sb->st_mtim.tv_sec);
    LL_MUL(info->modifyTime, info->modifyTime, s2us);
    LL_I2L(us, sb->st_mtim.tv_nsec / 1000);
    LL_ADD(info->modifyTime, info->modifyTime, us);
    LL_I2L(info->creationTime, sb->st_ctim.tv_sec);
    LL_MUL(info->creationTime, info->creationTime, s2us);
    LL_I2L(us, sb->st_ctim.tv_nsec / 1000);
    LL_ADD(info->creationTime, info->creationTime, us);
}

static void _MD_set_fileinfo64_times(
    const _MDStat64 *sb,
    PRFileInfo64 *info)
{
    PRInt64 us, s2us;

    LL_I2L(s2us, PR_USEC_PER_SEC);
    LL_I2L(info->modifyTime, sb->st_mtim.tv_sec);
    LL_MUL(info->modifyTime, info->modifyTime, s2us);
    LL_I2L(us, sb->st_mtim.tv_nsec / 1000);
    LL_ADD(info->modifyTime, info->modifyTime, us);
    LL_I2L(info->creationTime, sb->st_ctim.tv_sec);
    LL_MUL(info->creationTime, info->creationTime, s2us);
    LL_I2L(us, sb->st_ctim.tv_nsec / 1000);
    LL_ADD(info->creationTime, info->creationTime, us);
}
#elif defined(_PR_STAT_HAS_ST_ATIM_UNION)




static void _MD_set_fileinfo_times(
    const struct stat *sb,
    PRFileInfo *info)
{
    PRInt64 us, s2us;

    LL_I2L(s2us, PR_USEC_PER_SEC);
    LL_I2L(info->modifyTime, sb->st_mtim.st__tim.tv_sec);
    LL_MUL(info->modifyTime, info->modifyTime, s2us);
    LL_I2L(us, sb->st_mtim.st__tim.tv_nsec / 1000);
    LL_ADD(info->modifyTime, info->modifyTime, us);
    LL_I2L(info->creationTime, sb->st_ctim.st__tim.tv_sec);
    LL_MUL(info->creationTime, info->creationTime, s2us);
    LL_I2L(us, sb->st_ctim.st__tim.tv_nsec / 1000);
    LL_ADD(info->creationTime, info->creationTime, us);
}

static void _MD_set_fileinfo64_times(
    const _MDStat64 *sb,
    PRFileInfo64 *info)
{
    PRInt64 us, s2us;

    LL_I2L(s2us, PR_USEC_PER_SEC);
    LL_I2L(info->modifyTime, sb->st_mtim.st__tim.tv_sec);
    LL_MUL(info->modifyTime, info->modifyTime, s2us);
    LL_I2L(us, sb->st_mtim.st__tim.tv_nsec / 1000);
    LL_ADD(info->modifyTime, info->modifyTime, us);
    LL_I2L(info->creationTime, sb->st_ctim.st__tim.tv_sec);
    LL_MUL(info->creationTime, info->creationTime, s2us);
    LL_I2L(us, sb->st_ctim.st__tim.tv_nsec / 1000);
    LL_ADD(info->creationTime, info->creationTime, us);
}
#elif defined(_PR_STAT_HAS_ST_ATIMESPEC)




#if defined(_PR_TIMESPEC_HAS_TS_SEC)
static void _MD_set_fileinfo_times(
    const struct stat *sb,
    PRFileInfo *info)
{
    PRInt64 us, s2us;

    LL_I2L(s2us, PR_USEC_PER_SEC);
    LL_I2L(info->modifyTime, sb->st_mtimespec.ts_sec);
    LL_MUL(info->modifyTime, info->modifyTime, s2us);
    LL_I2L(us, sb->st_mtimespec.ts_nsec / 1000);
    LL_ADD(info->modifyTime, info->modifyTime, us);
    LL_I2L(info->creationTime, sb->st_ctimespec.ts_sec);
    LL_MUL(info->creationTime, info->creationTime, s2us);
    LL_I2L(us, sb->st_ctimespec.ts_nsec / 1000);
    LL_ADD(info->creationTime, info->creationTime, us);
}

static void _MD_set_fileinfo64_times(
    const _MDStat64 *sb,
    PRFileInfo64 *info)
{
    PRInt64 us, s2us;

    LL_I2L(s2us, PR_USEC_PER_SEC);
    LL_I2L(info->modifyTime, sb->st_mtimespec.ts_sec);
    LL_MUL(info->modifyTime, info->modifyTime, s2us);
    LL_I2L(us, sb->st_mtimespec.ts_nsec / 1000);
    LL_ADD(info->modifyTime, info->modifyTime, us);
    LL_I2L(info->creationTime, sb->st_ctimespec.ts_sec);
    LL_MUL(info->creationTime, info->creationTime, s2us);
    LL_I2L(us, sb->st_ctimespec.ts_nsec / 1000);
    LL_ADD(info->creationTime, info->creationTime, us);
}
#else 



static void _MD_set_fileinfo_times(
    const struct stat *sb,
    PRFileInfo *info)
{
    PRInt64 us, s2us;

    LL_I2L(s2us, PR_USEC_PER_SEC);
    LL_I2L(info->modifyTime, sb->st_mtimespec.tv_sec);
    LL_MUL(info->modifyTime, info->modifyTime, s2us);
    LL_I2L(us, sb->st_mtimespec.tv_nsec / 1000);
    LL_ADD(info->modifyTime, info->modifyTime, us);
    LL_I2L(info->creationTime, sb->st_ctimespec.tv_sec);
    LL_MUL(info->creationTime, info->creationTime, s2us);
    LL_I2L(us, sb->st_ctimespec.tv_nsec / 1000);
    LL_ADD(info->creationTime, info->creationTime, us);
}

static void _MD_set_fileinfo64_times(
    const _MDStat64 *sb,
    PRFileInfo64 *info)
{
    PRInt64 us, s2us;

    LL_I2L(s2us, PR_USEC_PER_SEC);
    LL_I2L(info->modifyTime, sb->st_mtimespec.tv_sec);
    LL_MUL(info->modifyTime, info->modifyTime, s2us);
    LL_I2L(us, sb->st_mtimespec.tv_nsec / 1000);
    LL_ADD(info->modifyTime, info->modifyTime, us);
    LL_I2L(info->creationTime, sb->st_ctimespec.tv_sec);
    LL_MUL(info->creationTime, info->creationTime, s2us);
    LL_I2L(us, sb->st_ctimespec.tv_nsec / 1000);
    LL_ADD(info->creationTime, info->creationTime, us);
}
#endif 
#elif defined(_PR_STAT_HAS_ONLY_ST_ATIME)




static void _MD_set_fileinfo_times(
    const struct stat *sb,
    PRFileInfo *info)
{
    PRInt64 s, s2us;
    LL_I2L(s2us, PR_USEC_PER_SEC);
    LL_I2L(s, sb->st_mtime);
    LL_MUL(s, s, s2us);
    info->modifyTime = s;
    LL_I2L(s, sb->st_ctime);
    LL_MUL(s, s, s2us);
    info->creationTime = s;
}

static void _MD_set_fileinfo64_times(
    const _MDStat64 *sb,
    PRFileInfo64 *info)
{
    PRInt64 s, s2us;
    LL_I2L(s2us, PR_USEC_PER_SEC);
    LL_I2L(s, sb->st_mtime);
    LL_MUL(s, s, s2us);
    info->modifyTime = s;
    LL_I2L(s, sb->st_ctime);
    LL_MUL(s, s, s2us);
    info->creationTime = s;
}
#else
#error "I don't know yet"
#endif

static int _MD_convert_stat_to_fileinfo(
    const struct stat *sb,
    PRFileInfo *info)
{
    if (S_IFREG & sb->st_mode)
        info->type = PR_FILE_FILE;
    else if (S_IFDIR & sb->st_mode)
        info->type = PR_FILE_DIRECTORY;
    else
        info->type = PR_FILE_OTHER;

#if defined(_PR_HAVE_LARGE_OFF_T)
    if (0x7fffffffL < sb->st_size)
    {
        PR_SetError(PR_FILE_TOO_BIG_ERROR, 0);
        return -1;
    }
#endif 
    info->size = sb->st_size;

    _MD_set_fileinfo_times(sb, info);
    return 0;
}  

static int _MD_convert_stat64_to_fileinfo64(
    const _MDStat64 *sb,
    PRFileInfo64 *info)
{
    if (S_IFREG & sb->st_mode)
        info->type = PR_FILE_FILE;
    else if (S_IFDIR & sb->st_mode)
        info->type = PR_FILE_DIRECTORY;
    else
        info->type = PR_FILE_OTHER;

    LL_I2L(info->size, sb->st_size);

    _MD_set_fileinfo64_times(sb, info);
    return 0;
}  

PRInt32 _MD_getfileinfo(const char *fn, PRFileInfo *info)
{
    PRInt32 rv;
    struct stat sb;

    rv = stat(fn, &sb);
    if (rv < 0)
        _PR_MD_MAP_STAT_ERROR(_MD_ERRNO());
    else if (NULL != info)
        rv = _MD_convert_stat_to_fileinfo(&sb, info);
    return rv;
}

PRInt32 _MD_getfileinfo64(const char *fn, PRFileInfo64 *info)
{
    _MDStat64 sb;
    PRInt32 rv = _md_iovector._stat64(fn, &sb);
    if (rv < 0)
        _PR_MD_MAP_STAT_ERROR(_MD_ERRNO());
    else if (NULL != info)
        rv = _MD_convert_stat64_to_fileinfo64(&sb, info);
    return rv;
}

PRInt32 _MD_getopenfileinfo(const PRFileDesc *fd, PRFileInfo *info)
{
    struct stat sb;
    PRInt32 rv = fstat(fd->secret->md.osfd, &sb);
    if (rv < 0)
        _PR_MD_MAP_FSTAT_ERROR(_MD_ERRNO());
    else if (NULL != info)
        rv = _MD_convert_stat_to_fileinfo(&sb, info);
    return rv;
}

PRInt32 _MD_getopenfileinfo64(const PRFileDesc *fd, PRFileInfo64 *info)
{
    _MDStat64 sb;
    PRInt32 rv = _md_iovector._fstat64(fd->secret->md.osfd, &sb);
    if (rv < 0)
        _PR_MD_MAP_FSTAT_ERROR(_MD_ERRNO());
    else if (NULL != info)
        rv = _MD_convert_stat64_to_fileinfo64(&sb, info);
    return rv;
}

struct _MD_IOVector _md_iovector;









#if defined(SOLARIS2_5)

static PRIntn _MD_solaris25_fstat64(PRIntn osfd, _MDStat64 *buf)
{
    PRInt32 rv;
    struct stat sb;

    rv = fstat(osfd, &sb);
    if (rv >= 0)
    {
        




        (void)memset(buf, 0, sizeof(_MDStat64));
        buf->st_mode = sb.st_mode;
        buf->st_ctim = sb.st_ctim;
        buf->st_mtim = sb.st_mtim;
        buf->st_size = sb.st_size;
    }
    return rv;
}  

static PRIntn _MD_solaris25_stat64(const char *fn, _MDStat64 *buf)
{
    PRInt32 rv;
    struct stat sb;

    rv = stat(fn, &sb);
    if (rv >= 0)
    {
        




        (void)memset(buf, 0, sizeof(_MDStat64));
        buf->st_mode = sb.st_mode;
        buf->st_ctim = sb.st_ctim;
        buf->st_mtim = sb.st_mtim;
        buf->st_size = sb.st_size;
    }
    return rv;
}  
#endif 

#if defined(_PR_NO_LARGE_FILES) || defined(SOLARIS2_5)

static PROffset64 _MD_Unix_lseek64(PRIntn osfd, PROffset64 offset, PRIntn whence)
{
    PRUint64 maxoff;
    PROffset64 rv = minus_one;
    LL_I2L(maxoff, 0x7fffffff);
    if (LL_CMP(offset, <=, maxoff))
    {
        off_t off;
        LL_L2I(off, offset);
        LL_I2L(rv, lseek(osfd, off, whence));
    }
    else errno = EFBIG;  
    return rv;
}  

static void* _MD_Unix_mmap64(
    void *addr, PRSize len, PRIntn prot, PRIntn flags,
    PRIntn fildes, PRInt64 offset)
{
    PR_SetError(PR_FILE_TOO_BIG_ERROR, 0);
    return NULL;
}  
#endif 

#if defined(OSF1) && defined(__GNUC__)









static int stat_forwarder(const char *path, struct stat *buffer)
{
    return stat(path, buffer);
}

static int fstat_forwarder(int filedes, struct stat *buffer)
{
    return fstat(filedes, buffer);
}

#endif

static void _PR_InitIOV(void)
{
#if defined(SOLARIS2_5)
    PRLibrary *lib;
    void *open64_func;

    open64_func = PR_FindSymbolAndLibrary("open64", &lib);
    if (NULL != open64_func)
    {
        PR_ASSERT(NULL != lib);
        _md_iovector._open64 = (_MD_Open64)open64_func;
        _md_iovector._mmap64 = (_MD_Mmap64)PR_FindSymbol(lib, "mmap64");
        _md_iovector._fstat64 = (_MD_Fstat64)PR_FindSymbol(lib, "fstat64");
        _md_iovector._stat64 = (_MD_Stat64)PR_FindSymbol(lib, "stat64");
        _md_iovector._lseek64 = (_MD_Lseek64)PR_FindSymbol(lib, "lseek64");
        (void)PR_UnloadLibrary(lib);
    }
    else
    {
        _md_iovector._open64 = open;
        _md_iovector._mmap64 = _MD_Unix_mmap64;
        _md_iovector._fstat64 = _MD_solaris25_fstat64;
        _md_iovector._stat64 = _MD_solaris25_stat64;
        _md_iovector._lseek64 = _MD_Unix_lseek64;
    }
#elif defined(_PR_NO_LARGE_FILES)
    _md_iovector._open64 = open;
    _md_iovector._mmap64 = _MD_Unix_mmap64;
    _md_iovector._fstat64 = fstat;
    _md_iovector._stat64 = stat;
    _md_iovector._lseek64 = _MD_Unix_lseek64;
#elif defined(_PR_HAVE_OFF64_T)
#if defined(IRIX5_3)
    _md_iovector._open64 = open;
#else
    _md_iovector._open64 = open64;
#endif
    _md_iovector._mmap64 = mmap64;
    _md_iovector._fstat64 = fstat64;
    _md_iovector._stat64 = stat64;
    _md_iovector._lseek64 = lseek64;
#elif defined(_PR_HAVE_LARGE_OFF_T)
    _md_iovector._open64 = open;
    _md_iovector._mmap64 = mmap;
#if defined(OSF1) && defined(__GNUC__)
    _md_iovector._fstat64 = fstat_forwarder;
    _md_iovector._stat64 = stat_forwarder;
#else
    _md_iovector._fstat64 = fstat;
    _md_iovector._stat64 = stat;
#endif
    _md_iovector._lseek64 = lseek;
#else
#error "I don't know yet"
#endif
    LL_I2L(minus_one, -1);
}  

void _PR_UnixInit(void)
{
    struct sigaction sigact;
    int rv;

    sigemptyset(&timer_set);

#if !defined(_PR_PTHREADS)

    sigaddset(&timer_set, SIGALRM);
    sigemptyset(&empty_set);
    intr_timeout_ticks =
            PR_SecondsToInterval(_PR_INTERRUPT_CHECK_INTERVAL_SECS);

#if defined(SOLARIS) || defined(IRIX)

    if (getenv("NSPR_SIGSEGV_HANDLE")) {
        sigact.sa_handler = sigsegvhandler;
        sigact.sa_flags = 0;
        sigact.sa_mask = timer_set;
        sigaction(SIGSEGV, &sigact, 0);
    }

    if (getenv("NSPR_SIGABRT_HANDLE")) {
        sigact.sa_handler = sigaborthandler;
        sigact.sa_flags = 0;
        sigact.sa_mask = timer_set;
        sigaction(SIGABRT, &sigact, 0);
    }

    if (getenv("NSPR_SIGBUS_HANDLE")) {
        sigact.sa_handler = sigbushandler;
        sigact.sa_flags = 0;
        sigact.sa_mask = timer_set;
        sigaction(SIGBUS, &sigact, 0);
    }

#endif
#endif  

    




#if defined(HPUX) && defined(_PR_DCETHREADS)
    {
        struct sigvec vec;

        vec.sv_handler = SIG_IGN;
        vec.sv_mask = 0;
        vec.sv_flags = 0;
        rv = sigvector(SIGPIPE, &vec, NULL);
        PR_ASSERT(0 == rv);
    }
#else
    sigact.sa_handler = SIG_IGN;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    rv = sigaction(SIGPIPE, &sigact, 0);
    PR_ASSERT(0 == rv);
#endif 

    _pr_rename_lock = PR_NewLock();
    PR_ASSERT(NULL != _pr_rename_lock);
    _pr_Xfe_mon = PR_NewMonitor();
    PR_ASSERT(NULL != _pr_Xfe_mon);

    _PR_InitIOV();  
}

#if !defined(_PR_PTHREADS)




static PRInt32 _pr_zero_fd = -1;
static PRLock *_pr_md_lock = NULL;








void _MD_InitSegs(void)
{
#ifdef DEBUG
    


    if (getenv("NSPR_NO_MMAP")) {
        _pr_zero_fd = -2;
        return;
    }
#endif
    _pr_zero_fd = open("/dev/zero",O_RDWR , 0);
    
    fcntl(_pr_zero_fd, F_SETFD, FD_CLOEXEC);
    _pr_md_lock = PR_NewLock();
}

PRStatus _MD_AllocSegment(PRSegment *seg, PRUint32 size, void *vaddr)
{
    static char *lastaddr = (char*) _PR_STACK_VMBASE;
    PRStatus retval = PR_SUCCESS;
    int prot;
    void *rv;

    PR_ASSERT(seg != 0);
    PR_ASSERT(size != 0);

    PR_Lock(_pr_md_lock);
    if (_pr_zero_fd < 0) {
from_heap:
        seg->vaddr = PR_MALLOC(size);
        if (!seg->vaddr) {
            retval = PR_FAILURE;
        }
        else {
            seg->size = size;
        }
        goto exit;
    }

    prot = PROT_READ|PROT_WRITE;
    




#if defined(LINUX) && defined(__alpha)
    prot |= PROT_EXEC;
#endif
    rv = mmap((vaddr != 0) ? vaddr : lastaddr, size, prot,
        _MD_MMAP_FLAGS,
        _pr_zero_fd, 0);
    if (rv == (void*)-1) {
        goto from_heap;
    }
    lastaddr += size;
    seg->vaddr = rv;
    seg->size = size;
    seg->flags = _PR_SEG_VM;

exit:
    PR_Unlock(_pr_md_lock);
    return retval;
}

void _MD_FreeSegment(PRSegment *seg)
{
    if (seg->flags & _PR_SEG_VM)
        (void) munmap(seg->vaddr, seg->size);
    else
        PR_DELETE(seg->vaddr);
}

#endif 















PR_IMPLEMENT(PRTime)
PR_Now(void)
{
    struct timeval tv;
    PRInt64 s, us, s2us;

    GETTIMEOFDAY(&tv);
    LL_I2L(s2us, PR_USEC_PER_SEC);
    LL_I2L(s, tv.tv_sec);
    LL_I2L(us, tv.tv_usec);
    LL_MUL(s, s, s2us);
    LL_ADD(s, s, us);
    return s;
}

PRIntervalTime _PR_UNIX_GetInterval()
{
    struct timeval time;
    PRIntervalTime ticks;

    (void)GETTIMEOFDAY(&time);  
    ticks = (PRUint32)time.tv_sec * PR_MSEC_PER_SEC;  
    ticks += (PRUint32)time.tv_usec / PR_USEC_PER_MSEC;  
    return ticks;
}  

PRIntervalTime _PR_UNIX_TicksPerSecond()
{
    return 1000;  
}

#if !defined(_PR_PTHREADS)






PRInt32 _PR_WaitForMultipleFDs(
    _PRUnixPollDesc *unixpds,
    PRInt32 pdcnt,
    PRIntervalTime timeout)
{
    PRPollQueue pq;
    PRIntn is;
    PRInt32 rv;
    _PRCPU *io_cpu;
    _PRUnixPollDesc *unixpd, *eunixpd;
    PRThread *me = _PR_MD_CURRENT_THREAD();

    PR_ASSERT(!(me->flags & _PR_IDLE_THREAD));

    if (_PR_PENDING_INTERRUPT(me)) {
        me->flags &= ~_PR_INTERRUPT;
        PR_SetError(PR_PENDING_INTERRUPT_ERROR, 0);
        return -1;
    }

    pq.pds = unixpds;
    pq.npds = pdcnt;

    _PR_INTSOFF(is);
    _PR_MD_IOQ_LOCK();
    _PR_THREAD_LOCK(me);

    pq.thr = me;
    io_cpu = me->cpu;
    pq.on_ioq = PR_TRUE;
    pq.timeout = timeout;
    _PR_ADD_TO_IOQ(pq, me->cpu);

#if !defined(_PR_USE_POLL)
    eunixpd = unixpds + pdcnt;
    for (unixpd = unixpds; unixpd < eunixpd; unixpd++) {
        PRInt32 osfd = unixpd->osfd;
        if (unixpd->in_flags & _PR_UNIX_POLL_READ) {
            FD_SET(osfd, &_PR_FD_READ_SET(me->cpu));
            _PR_FD_READ_CNT(me->cpu)[osfd]++;
        }
        if (unixpd->in_flags & _PR_UNIX_POLL_WRITE) {
            FD_SET(osfd, &_PR_FD_WRITE_SET(me->cpu));
            (_PR_FD_WRITE_CNT(me->cpu))[osfd]++;
        }
        if (unixpd->in_flags & _PR_UNIX_POLL_EXCEPT) {
            FD_SET(osfd, &_PR_FD_EXCEPTION_SET(me->cpu));
            (_PR_FD_EXCEPTION_CNT(me->cpu))[osfd]++;
        }
        if (osfd > _PR_IOQ_MAX_OSFD(me->cpu)) {
            _PR_IOQ_MAX_OSFD(me->cpu) = osfd;
        }
    }
#endif  

    if (_PR_IOQ_TIMEOUT(me->cpu) > timeout) {
        _PR_IOQ_TIMEOUT(me->cpu) = timeout;
    }

    _PR_IOQ_OSFD_CNT(me->cpu) += pdcnt;
        
    _PR_SLEEPQ_LOCK(me->cpu);
    _PR_ADD_SLEEPQ(me, timeout);
    me->state = _PR_IO_WAIT;
    me->io_pending = PR_TRUE;
    me->io_suspended = PR_FALSE;
    _PR_SLEEPQ_UNLOCK(me->cpu);
    _PR_THREAD_UNLOCK(me);
    _PR_MD_IOQ_UNLOCK();

    _PR_MD_WAIT(me, timeout);

    me->io_pending = PR_FALSE;
    me->io_suspended = PR_FALSE;

    




    PR_ASSERT(me->cpu == io_cpu);

    



    if (pq.on_ioq) {
        _PR_MD_IOQ_LOCK();
        


        if (pq.on_ioq) {
            PR_REMOVE_LINK(&pq.links);
#ifndef _PR_USE_POLL
            eunixpd = unixpds + pdcnt;
            for (unixpd = unixpds; unixpd < eunixpd; unixpd++) {
                PRInt32 osfd = unixpd->osfd;
                PRInt16 in_flags = unixpd->in_flags;

                if (in_flags & _PR_UNIX_POLL_READ) {
                    if (--(_PR_FD_READ_CNT(me->cpu))[osfd] == 0)
                        FD_CLR(osfd, &_PR_FD_READ_SET(me->cpu));
                }
                if (in_flags & _PR_UNIX_POLL_WRITE) {
                    if (--(_PR_FD_WRITE_CNT(me->cpu))[osfd] == 0)
                        FD_CLR(osfd, &_PR_FD_WRITE_SET(me->cpu));
                }
                if (in_flags & _PR_UNIX_POLL_EXCEPT) {
                    if (--(_PR_FD_EXCEPTION_CNT(me->cpu))[osfd] == 0)
                        FD_CLR(osfd, &_PR_FD_EXCEPTION_SET(me->cpu));
                }
            }
#endif  
            PR_ASSERT(pq.npds == pdcnt);
            _PR_IOQ_OSFD_CNT(me->cpu) -= pdcnt;
            PR_ASSERT(_PR_IOQ_OSFD_CNT(me->cpu) >= 0);
        }
        _PR_MD_IOQ_UNLOCK();
    }
    
    if (1 == pdcnt) {
        _PR_FAST_INTSON(is);
    } else {
        _PR_INTSON(is);
    }

    if (_PR_PENDING_INTERRUPT(me)) {
        me->flags &= ~_PR_INTERRUPT;
        PR_SetError(PR_PENDING_INTERRUPT_ERROR, 0);
        return -1;
    }

    rv = 0;
    if (pq.on_ioq == PR_FALSE) {
        
        while (--pdcnt >= 0) {
            if (unixpds->out_flags != 0) {
                rv++;
            }
            unixpds++;
        }
    }

    return rv;
}








void _PR_Unblock_IO_Wait(PRThread *thr)
{
    int pri = thr->priority;
    _PRCPU *cpu = thr->cpu;
 
    


    if (_PR_IS_NATIVE_THREAD(thr)) {
        _PR_THREAD_UNLOCK(thr); 
        return;
    }

    PR_ASSERT(thr->flags & (_PR_ON_SLEEPQ | _PR_ON_PAUSEQ));
    _PR_SLEEPQ_LOCK(cpu);
    _PR_DEL_SLEEPQ(thr, PR_TRUE);
    _PR_SLEEPQ_UNLOCK(cpu);

    PR_ASSERT(!(thr->flags & _PR_IDLE_THREAD));
    thr->state = _PR_RUNNABLE;
    _PR_RUNQ_LOCK(cpu);
    _PR_ADD_RUNQ(thr, cpu, pri);
    _PR_RUNQ_UNLOCK(cpu);
    _PR_THREAD_UNLOCK(thr);
    _PR_MD_WAKEUP_WAITER(thr);
}
#endif  









int _MD_unix_get_nonblocking_connect_error(int osfd)
{
#if defined(NTO)
    
    PRInt32      rv;
    PRNetAddr    addr;
    _PRSockLen_t addrlen = sizeof(addr);

    
    struct statvfs superblock;
    rv = fstatvfs(osfd, &superblock);
    if (rv == 0) {
        if (strcmp(superblock.f_basetype, "ttcpip") == 0) {
            
            rv = getpeername(osfd, (struct sockaddr *) &addr,
                    (_PRSockLen_t *) &addrlen);
            if (rv == -1) {
                int errno_copy = errno;    


                if (errno_copy == ENOTCONN) {
                    struct stat StatInfo;
                    rv = fstat(osfd, &StatInfo);
                    if (rv == 0) {
                        time_t current_time = time(NULL);

                        



                        if (abs(current_time - StatInfo.st_atime) < 5) {
                            return ECONNREFUSED;
                        } else {
                            return ETIMEDOUT;
                        }
                    } else {
                        return ECONNREFUSED;
                    }
                } else {
                    return errno_copy;
                }
            } else {
                
                return 0;
            }
        } else {
            
            
            

            int err;
            _PRSockLen_t optlen = sizeof(err);

            if (getsockopt(osfd, SOL_SOCKET, SO_ERROR,
                    (char *) &err, &optlen) == -1) {
                return errno;
            } else {
                return err;
            }		
        }
    } else {
        return ECONNREFUSED;
    }	
#elif defined(NCR) || defined(UNIXWARE) || defined(SNI) || defined(NEC)
    



    int rv;
    int flags = 0;
    rv = getmsg(osfd, NULL, NULL, &flags);
    PR_ASSERT(-1 == rv || 0 == rv);
    if (-1 == rv && errno != EAGAIN && errno != EWOULDBLOCK) {
        return errno;
    }
    return 0;  
#else
    int err;
    _PRSockLen_t optlen = sizeof(err);
    if (getsockopt(osfd, SOL_SOCKET, SO_ERROR, (char *) &err, &optlen) == -1) {
        return errno;
    } else {
        return err;
    }
#endif
}









void PR_XLock(void)
{
    PR_EnterMonitor(_pr_Xfe_mon);
}

void PR_XUnlock(void)
{
    PR_ExitMonitor(_pr_Xfe_mon);
}

PRBool PR_XIsLocked(void)
{
    return (PR_InMonitor(_pr_Xfe_mon)) ? PR_TRUE : PR_FALSE;
}

void PR_XWait(int ms)
{
    PR_Wait(_pr_Xfe_mon, PR_MillisecondsToInterval(ms));
}

void PR_XNotify(void)
{
    PR_Notify(_pr_Xfe_mon);
}

void PR_XNotifyAll(void)
{
    PR_NotifyAll(_pr_Xfe_mon);
}

#if defined(HAVE_FCNTL_FILE_LOCKING)

PRStatus
_MD_LockFile(PRInt32 f)
{
    PRInt32 rv;
    struct flock arg;

    arg.l_type = F_WRLCK;
    arg.l_whence = SEEK_SET;
    arg.l_start = 0;
    arg.l_len = 0;  
    rv = fcntl(f, F_SETLKW, &arg);
    if (rv == 0)
        return PR_SUCCESS;
    _PR_MD_MAP_FLOCK_ERROR(_MD_ERRNO());
    return PR_FAILURE;
}

PRStatus
_MD_TLockFile(PRInt32 f)
{
    PRInt32 rv;
    struct flock arg;

    arg.l_type = F_WRLCK;
    arg.l_whence = SEEK_SET;
    arg.l_start = 0;
    arg.l_len = 0;  
    rv = fcntl(f, F_SETLK, &arg);
    if (rv == 0)
        return PR_SUCCESS;
    _PR_MD_MAP_FLOCK_ERROR(_MD_ERRNO());
    return PR_FAILURE;
}

PRStatus
_MD_UnlockFile(PRInt32 f)
{
    PRInt32 rv;
    struct flock arg;

    arg.l_type = F_UNLCK;
    arg.l_whence = SEEK_SET;
    arg.l_start = 0;
    arg.l_len = 0;  
    rv = fcntl(f, F_SETLK, &arg);
    if (rv == 0)
        return PR_SUCCESS;
    _PR_MD_MAP_FLOCK_ERROR(_MD_ERRNO());
    return PR_FAILURE;
}

#elif defined(HAVE_BSD_FLOCK)

#include <sys/file.h>

PRStatus
_MD_LockFile(PRInt32 f)
{
    PRInt32 rv;
    rv = flock(f, LOCK_EX);
    if (rv == 0)
        return PR_SUCCESS;
    _PR_MD_MAP_FLOCK_ERROR(_MD_ERRNO());
    return PR_FAILURE;
}

PRStatus
_MD_TLockFile(PRInt32 f)
{
    PRInt32 rv;
    rv = flock(f, LOCK_EX|LOCK_NB);
    if (rv == 0)
        return PR_SUCCESS;
    _PR_MD_MAP_FLOCK_ERROR(_MD_ERRNO());
    return PR_FAILURE;
}

PRStatus
_MD_UnlockFile(PRInt32 f)
{
    PRInt32 rv;
    rv = flock(f, LOCK_UN);
    if (rv == 0)
        return PR_SUCCESS;
    _PR_MD_MAP_FLOCK_ERROR(_MD_ERRNO());
    return PR_FAILURE;
}
#else

PRStatus
_MD_LockFile(PRInt32 f)
{
    PRInt32 rv;
    rv = lockf(f, F_LOCK, 0);
    if (rv == 0)
        return PR_SUCCESS;
    _PR_MD_MAP_LOCKF_ERROR(_MD_ERRNO());
    return PR_FAILURE;
}

PRStatus
_MD_TLockFile(PRInt32 f)
{
    PRInt32 rv;
    rv = lockf(f, F_TLOCK, 0);
    if (rv == 0)
        return PR_SUCCESS;
    _PR_MD_MAP_LOCKF_ERROR(_MD_ERRNO());
    return PR_FAILURE;
}

PRStatus
_MD_UnlockFile(PRInt32 f)
{
    PRInt32 rv;
    rv = lockf(f, F_ULOCK, 0);
    if (rv == 0)
        return PR_SUCCESS;
    _PR_MD_MAP_LOCKF_ERROR(_MD_ERRNO());
    return PR_FAILURE;
}
#endif

PRStatus _MD_gethostname(char *name, PRUint32 namelen)
{
    PRIntn rv;

    rv = gethostname(name, namelen);
    if (0 == rv) {
        return PR_SUCCESS;
    }
    _PR_MD_MAP_GETHOSTNAME_ERROR(_MD_ERRNO());
    return PR_FAILURE;
}

PRStatus _MD_getsysinfo(PRSysInfo cmd, char *name, PRUint32 namelen)
{
	struct utsname info;

	PR_ASSERT((cmd == PR_SI_SYSNAME) || (cmd == PR_SI_RELEASE));

	if (uname(&info) == -1) {
		_PR_MD_MAP_DEFAULT_ERROR(errno);
    	return PR_FAILURE;
	}
	if (PR_SI_SYSNAME == cmd)
		(void)PR_snprintf(name, namelen, info.sysname);
	else if (PR_SI_RELEASE == cmd)
		(void)PR_snprintf(name, namelen, info.release);
	else
		return PR_FAILURE;
    return PR_SUCCESS;
}









PRStatus _MD_CreateFileMap(PRFileMap *fmap, PRInt64 size)
{
    PRFileInfo info;
    PRUint32 sz;

    LL_L2UI(sz, size);
    if (sz) {
        if (PR_GetOpenFileInfo(fmap->fd, &info) == PR_FAILURE) {
            return PR_FAILURE;
        }
        if (sz > info.size) {
            


            if (fmap->prot != PR_PROT_READWRITE) {
                PR_SetError(PR_NO_ACCESS_RIGHTS_ERROR, 0);
                return PR_FAILURE;
            }
            if (PR_Seek(fmap->fd, sz - 1, PR_SEEK_SET) == -1) {
                return PR_FAILURE;
            }
            if (PR_Write(fmap->fd, "", 1) != 1) {
                return PR_FAILURE;
            }
        }
    }
    if (fmap->prot == PR_PROT_READONLY) {
        fmap->md.prot = PROT_READ;
#ifdef OSF1V4_MAP_PRIVATE_BUG
        





        fmap->md.flags = MAP_SHARED;
#else
        fmap->md.flags = MAP_PRIVATE;
#endif
    } else if (fmap->prot == PR_PROT_READWRITE) {
        fmap->md.prot = PROT_READ | PROT_WRITE;
        fmap->md.flags = MAP_SHARED;
    } else {
        PR_ASSERT(fmap->prot == PR_PROT_WRITECOPY);
        fmap->md.prot = PROT_READ | PROT_WRITE;
        fmap->md.flags = MAP_PRIVATE;
    }
    return PR_SUCCESS;
}

void * _MD_MemMap(
    PRFileMap *fmap,
    PRInt64 offset,
    PRUint32 len)
{
    PRInt32 off;
    void *addr;

    LL_L2I(off, offset);
    if ((addr = mmap(0, len, fmap->md.prot, fmap->md.flags,
        fmap->fd->secret->md.osfd, off)) == (void *) -1) {
            _PR_MD_MAP_MMAP_ERROR(_MD_ERRNO());
        addr = NULL;
    }
    return addr;
}

PRStatus _MD_MemUnmap(void *addr, PRUint32 len)
{
    if (munmap(addr, len) == 0) {
        return PR_SUCCESS;
    } else {
    if (errno == EINVAL) {
            PR_SetError(PR_INVALID_ARGUMENT_ERROR, errno);
    } else {
        PR_SetError(PR_UNKNOWN_ERROR, errno);
    }
        return PR_FAILURE;
    }
}

PRStatus _MD_CloseFileMap(PRFileMap *fmap)
{
    if ( PR_TRUE == fmap->md.isAnonFM ) {
        PRStatus rc = PR_Close( fmap->fd );
        if ( PR_FAILURE == rc ) {
            PR_LOG( _pr_io_lm, PR_LOG_DEBUG,
                ("_MD_CloseFileMap(): error closing anonymnous file map osfd"));
            return PR_FAILURE;
        }
    }
    PR_DELETE(fmap);
    return PR_SUCCESS;
}

#if defined(_PR_NEED_FAKE_POLL)






int poll(struct pollfd *filedes, unsigned long nfds, int timeout)
{
    int i;
    int rv;
    int maxfd;
    fd_set rd, wr, ex;
    struct timeval tv, *tvp;

    if (timeout < 0 && timeout != -1) {
        errno = EINVAL;
        return -1;
    }

    if (timeout == -1) {
        tvp = NULL;
    } else {
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        tvp = &tv;
    }

    maxfd = -1;
    FD_ZERO(&rd);
    FD_ZERO(&wr);
    FD_ZERO(&ex);

    for (i = 0; i < nfds; i++) {
        int osfd = filedes[i].fd;
        int events = filedes[i].events;
        PRBool fdHasEvent = PR_FALSE;

        if (osfd < 0) {
            continue;  
        }

        











        if (events & (POLLIN | POLLRDNORM)) {
            FD_SET(osfd, &rd);
            fdHasEvent = PR_TRUE;
        }
        if (events & (POLLOUT | POLLWRNORM)) {
            FD_SET(osfd, &wr);
            fdHasEvent = PR_TRUE;
        }
        if (events & (POLLPRI | POLLRDBAND)) {
            FD_SET(osfd, &ex);
            fdHasEvent = PR_TRUE;
        }
        if (fdHasEvent && osfd > maxfd) {
            maxfd = osfd;
        }
    }

    rv = select(maxfd + 1, &rd, &wr, &ex, tvp);

    
    if (rv > 0) {
        rv = 0;
        for (i = 0; i < nfds; i++) {
            PRBool fdHasEvent = PR_FALSE;

            filedes[i].revents = 0;
            if (filedes[i].fd < 0) {
                continue;
            }
            if (FD_ISSET(filedes[i].fd, &rd)) {
                if (filedes[i].events & POLLIN) {
                    filedes[i].revents |= POLLIN;
                }
                if (filedes[i].events & POLLRDNORM) {
                    filedes[i].revents |= POLLRDNORM;
                }
                fdHasEvent = PR_TRUE;
            }
            if (FD_ISSET(filedes[i].fd, &wr)) {
                if (filedes[i].events & POLLOUT) {
                    filedes[i].revents |= POLLOUT;
                }
                if (filedes[i].events & POLLWRNORM) {
                    filedes[i].revents |= POLLWRNORM;
                }
                fdHasEvent = PR_TRUE;
            }
            if (FD_ISSET(filedes[i].fd, &ex)) {
                if (filedes[i].events & POLLPRI) {
                    filedes[i].revents |= POLLPRI;
                }
                if (filedes[i].events & POLLRDBAND) {
                    filedes[i].revents |= POLLRDBAND;
                }
                fdHasEvent = PR_TRUE;
            }
            if (fdHasEvent) {
                rv++;
            }
        }
        PR_ASSERT(rv > 0);
    } else if (rv == -1 && errno == EBADF) {
        rv = 0;
        for (i = 0; i < nfds; i++) {
            filedes[i].revents = 0;
            if (filedes[i].fd < 0) {
                continue;
            }
            if (fcntl(filedes[i].fd, F_GETFL, 0) == -1) {
                filedes[i].revents = POLLNVAL;
                rv++;
            }
        }
        PR_ASSERT(rv > 0);
    }
    PR_ASSERT(-1 != timeout || rv != 0);

    return rv;
}
#endif 
