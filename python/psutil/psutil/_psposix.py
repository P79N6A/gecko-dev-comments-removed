







"""Routines common to all posix systems."""

import os
import errno
import psutil
import sys
import time
import glob

from psutil.error import TimeoutExpired
from psutil._common import nt_diskinfo, usage_percent


def pid_exists(pid):
    """Check whether pid exists in the current process table."""
    if not isinstance(pid, int):
        raise TypeError('an integer is required')
    if pid < 0:
        return False
    try:
        os.kill(pid, 0)
    except OSError:
        e = sys.exc_info()[1]
        return e.errno == errno.EPERM
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
            if time.time() >= stop_at:
                raise TimeoutExpired(pid)
        time.sleep(delay)
        return min(delay * 2, 0.04)

    if timeout is not None:
        waitcall = lambda: os.waitpid(pid, os.WNOHANG)
        stop_at = time.time() + timeout
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

def get_disk_usage(path):
    """Return disk usage associated with path."""
    st = os.statvfs(path)
    free = (st.f_bavail * st.f_frsize)
    total = (st.f_blocks * st.f_frsize)
    used = (st.f_blocks - st.f_bfree) * st.f_frsize
    percent = usage_percent(used, total, _round=1)
    
    
    
    return nt_diskinfo(total, used, free, percent)

def _get_terminal_map():
    ret = {}
    ls = glob.glob('/dev/tty*') + glob.glob('/dev/pts/*')
    for name in ls:
        assert name not in ret
        ret[os.stat(name).st_rdev] = name
    return ret
