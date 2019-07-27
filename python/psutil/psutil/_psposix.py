





"""Routines common to all posix systems."""

import errno
import glob
import os
import sys
import time

from psutil._common import sdiskusage, usage_percent, memoize
from psutil._compat import PY3, unicode


class TimeoutExpired(Exception):
    pass


def pid_exists(pid):
    """Check whether pid exists in the current process table."""
    if pid == 0:
        
        
        
        
        
        return True
    try:
        os.kill(pid, 0)
    except OSError:
        err = sys.exc_info()[1]
        if err.errno == errno.ESRCH:
            
            return False
        elif err.errno == errno.EPERM:
            
            return True
        else:
            
            
            
            
            raise err
    else:
        return True


def wait_pid(pid, timeout=None):
    """Wait for process with pid 'pid' to terminate and return its
    exit status code as an integer.

    If pid is not a children of os.getpid() (current process) just
    waits until the process disappears and return None.

    If pid does not exist at all return None immediately.

    Raise TimeoutExpired on timeout expired.
    """
    def check_timeout(delay):
        if timeout is not None:
            if timer() >= stop_at:
                raise TimeoutExpired()
        time.sleep(delay)
        return min(delay * 2, 0.04)

    timer = getattr(time, 'monotonic', time.time)
    if timeout is not None:
        waitcall = lambda: os.waitpid(pid, os.WNOHANG)
        stop_at = timer() + timeout
    else:
        waitcall = lambda: os.waitpid(pid, 0)

    delay = 0.0001
    while 1:
        try:
            retpid, status = waitcall()
        except OSError:
            err = sys.exc_info()[1]
            if err.errno == errno.EINTR:
                delay = check_timeout(delay)
                continue
            elif err.errno == errno.ECHILD:
                
                
                
                
                
                
                while 1:
                    if pid_exists(pid):
                        delay = check_timeout(delay)
                    else:
                        return
            else:
                raise
        else:
            if retpid == 0:
                
                delay = check_timeout(delay)
                continue
            
            
            if os.WIFSIGNALED(status):
                return os.WTERMSIG(status)
            
            
            elif os.WIFEXITED(status):
                return os.WEXITSTATUS(status)
            else:
                
                raise RuntimeError("unknown process exit status")


def disk_usage(path):
    """Return disk usage associated with path."""
    try:
        st = os.statvfs(path)
    except UnicodeEncodeError:
        if not PY3 and isinstance(path, unicode):
            
            
            
            
            try:
                path = path.encode(sys.getfilesystemencoding())
            except UnicodeEncodeError:
                pass
            st = os.statvfs(path)
        else:
            raise
    free = (st.f_bavail * st.f_frsize)
    total = (st.f_blocks * st.f_frsize)
    used = (st.f_blocks - st.f_bfree) * st.f_frsize
    percent = usage_percent(used, total, _round=1)
    
    
    
    return sdiskusage(total, used, free, percent)


@memoize
def _get_terminal_map():
    ret = {}
    ls = glob.glob('/dev/tty*') + glob.glob('/dev/pts/*')
    for name in ls:
        assert name not in ret
        try:
            ret[os.stat(name).st_rdev] = name
        except OSError:
            err = sys.exc_info()[1]
            if err.errno != errno.ENOENT:
                raise
    return ret
