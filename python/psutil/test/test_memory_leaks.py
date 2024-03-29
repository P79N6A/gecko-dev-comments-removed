





"""
A test script which attempts to detect memory leaks by calling C
functions many times and compare process memory usage before and
after the calls.  It might produce false positives.
"""

import gc
import os
import socket
import sys
import threading
import time

if sys.version_info < (2, 7):
    import unittest2 as unittest  
else:
    import unittest

import psutil
import psutil._common

from psutil._compat import callable, xrange
from test_psutil import (WINDOWS, POSIX, OSX, LINUX, SUNOS, TESTFN,
                         RLIMIT_SUPPORT)
from test_psutil import (reap_children, supports_ipv6, safe_remove,
                         get_test_subprocess)


LOOPS = 1000
TOLERANCE = 4096
SKIP_PYTHON_IMPL = True


def skip_if_linux():
    return unittest.skipIf(LINUX and SKIP_PYTHON_IMPL,
                           "not worth being tested on LINUX (pure python)")


class Base(unittest.TestCase):
    proc = psutil.Process(os.getpid())

    def execute(self, function, *args, **kwargs):
        def call_many_times():
            for x in xrange(LOOPS - 1):
                self.call(function, *args, **kwargs)
            del x
            gc.collect()
            return self.get_mem()

        self.call(function, *args, **kwargs)
        self.assertEqual(gc.garbage, [])
        self.assertEqual(threading.active_count(), 1)

        
        
        rss1 = call_many_times()
        
        rss2 = call_many_times()

        difference = rss2 - rss1
        if difference > TOLERANCE:
            
            
            
            
            
            
            
            stop_at = time.time() + 3
            while 1:
                self.call(function, *args, **kwargs)
                if time.time() >= stop_at:
                    break
            del stop_at
            gc.collect()
            rss3 = self.get_mem()
            difference = rss3 - rss2
            if rss3 > rss2:
                self.fail("rss2=%s, rss3=%s, difference=%s"
                          % (rss2, rss3, difference))

    def get_mem(self):
        return psutil.Process(os.getpid()).memory_info()[0]

    def call(self, *args, **kwargs):
        raise NotImplementedError("must be implemented in subclass")


class TestProcessObjectLeaks(Base):
    """Test leaks of Process class methods and properties"""

    def setUp(self):
        gc.collect()

    def tearDown(self):
        reap_children()

    def call(self, function, *args, **kwargs):
        try:
            obj = getattr(self.proc, function)
            if callable(obj):
                obj(*args, **kwargs)
        except psutil.Error:
            pass

    @skip_if_linux()
    def test_name(self):
        self.execute('name')

    @skip_if_linux()
    def test_cmdline(self):
        self.execute('cmdline')

    @skip_if_linux()
    def test_exe(self):
        self.execute('exe')

    @skip_if_linux()
    def test_ppid(self):
        self.execute('ppid')

    @unittest.skipUnless(POSIX, "POSIX only")
    @skip_if_linux()
    def test_uids(self):
        self.execute('uids')

    @unittest.skipUnless(POSIX, "POSIX only")
    @skip_if_linux()
    def test_gids(self):
        self.execute('gids')

    @skip_if_linux()
    def test_status(self):
        self.execute('status')

    def test_nice_get(self):
        self.execute('nice')

    def test_nice_set(self):
        niceness = psutil.Process(os.getpid()).nice()
        self.execute('nice', niceness)

    @unittest.skipUnless(hasattr(psutil.Process, 'ionice'),
                         "Linux and Windows Vista only")
    def test_ionice_get(self):
        self.execute('ionice')

    @unittest.skipUnless(hasattr(psutil.Process, 'ionice'),
                         "Linux and Windows Vista only")
    def test_ionice_set(self):
        if WINDOWS:
            value = psutil.Process(os.getpid()).ionice()
            self.execute('ionice', value)
        else:
            self.execute('ionice', psutil.IOPRIO_CLASS_NONE)

    @unittest.skipIf(OSX, "feature not supported on this platform")
    @skip_if_linux()
    def test_io_counters(self):
        self.execute('io_counters')

    def test_username(self):
        self.execute('username')

    @skip_if_linux()
    def test_create_time(self):
        self.execute('create_time')

    @skip_if_linux()
    def test_num_threads(self):
        self.execute('num_threads')

    @unittest.skipUnless(WINDOWS, "Windows only")
    def test_num_handles(self):
        self.execute('num_handles')

    @unittest.skipUnless(POSIX, "POSIX only")
    @skip_if_linux()
    def test_num_fds(self):
        self.execute('num_fds')

    @skip_if_linux()
    def test_threads(self):
        self.execute('threads')

    @skip_if_linux()
    def test_cpu_times(self):
        self.execute('cpu_times')

    @skip_if_linux()
    def test_memory_info(self):
        self.execute('memory_info')

    @skip_if_linux()
    def test_memory_info_ex(self):
        self.execute('memory_info_ex')

    @unittest.skipUnless(POSIX, "POSIX only")
    @skip_if_linux()
    def test_terminal(self):
        self.execute('terminal')

    @unittest.skipIf(POSIX and SKIP_PYTHON_IMPL,
                     "not worth being tested on POSIX (pure python)")
    def test_resume(self):
        self.execute('resume')

    @skip_if_linux()
    def test_cwd(self):
        self.execute('cwd')

    @unittest.skipUnless(WINDOWS or LINUX, "Windows or Linux only")
    def test_cpu_affinity_get(self):
        self.execute('cpu_affinity')

    @unittest.skipUnless(WINDOWS or LINUX, "Windows or Linux only")
    def test_cpu_affinity_set(self):
        affinity = psutil.Process(os.getpid()).cpu_affinity()
        self.execute('cpu_affinity', affinity)

    @skip_if_linux()
    def test_open_files(self):
        safe_remove(TESTFN)  
        f = open(TESTFN, 'w')
        try:
            self.execute('open_files')
        finally:
            f.close()

    
    @unittest.skipIf(OSX, "OSX implementation is too slow")
    @skip_if_linux()
    def test_memory_maps(self):
        self.execute('memory_maps')

    @unittest.skipUnless(LINUX, "Linux only")
    @unittest.skipUnless(LINUX and RLIMIT_SUPPORT,
                         "only available on Linux >= 2.6.36")
    def test_rlimit_get(self):
        self.execute('rlimit', psutil.RLIMIT_NOFILE)

    @unittest.skipUnless(LINUX, "Linux only")
    @unittest.skipUnless(LINUX and RLIMIT_SUPPORT,
                         "only available on Linux >= 2.6.36")
    def test_rlimit_set(self):
        limit = psutil.Process().rlimit(psutil.RLIMIT_NOFILE)
        self.execute('rlimit', psutil.RLIMIT_NOFILE, limit)

    @skip_if_linux()
    
    @unittest.skipIf(WINDOWS, "tested later")
    def test_connections(self):
        def create_socket(family, type):
            sock = socket.socket(family, type)
            sock.bind(('', 0))
            if type == socket.SOCK_STREAM:
                sock.listen(1)
            return sock

        socks = []
        socks.append(create_socket(socket.AF_INET, socket.SOCK_STREAM))
        socks.append(create_socket(socket.AF_INET, socket.SOCK_DGRAM))
        if supports_ipv6():
            socks.append(create_socket(socket.AF_INET6, socket.SOCK_STREAM))
            socks.append(create_socket(socket.AF_INET6, socket.SOCK_DGRAM))
        if hasattr(socket, 'AF_UNIX'):
            safe_remove(TESTFN)
            s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            s.bind(TESTFN)
            s.listen(1)
            socks.append(s)
        kind = 'all'
        
        
        
        if SUNOS:
            kind = 'inet'
        try:
            self.execute('connections', kind=kind)
        finally:
            for s in socks:
                s.close()


p = get_test_subprocess()
DEAD_PROC = psutil.Process(p.pid)
DEAD_PROC.kill()
DEAD_PROC.wait()
del p


class TestProcessObjectLeaksZombie(TestProcessObjectLeaks):
    """Same as above but looks for leaks occurring when dealing with
    zombie processes raising NoSuchProcess exception.
    """
    proc = DEAD_PROC

    if not POSIX:
        def test_kill(self):
            self.execute('kill')

        def test_terminate(self):
            self.execute('terminate')

        def test_suspend(self):
            self.execute('suspend')

        def test_resume(self):
            self.execute('resume')

        def test_wait(self):
            self.execute('wait')


class TestModuleFunctionsLeaks(Base):
    """Test leaks of psutil module functions."""

    def setUp(self):
        gc.collect()

    def call(self, function, *args, **kwargs):
        obj = getattr(psutil, function)
        if callable(obj):
            obj(*args, **kwargs)

    @skip_if_linux()
    def test_cpu_count_logical(self):
        psutil.cpu_count = psutil._psplatform.cpu_count_logical
        self.execute('cpu_count')

    @skip_if_linux()
    def test_cpu_count_physical(self):
        psutil.cpu_count = psutil._psplatform.cpu_count_physical
        self.execute('cpu_count')

    @skip_if_linux()
    def test_boot_time(self):
        self.execute('boot_time')

    @unittest.skipIf(POSIX and SKIP_PYTHON_IMPL,
                     "not worth being tested on POSIX (pure python)")
    def test_pid_exists(self):
        self.execute('pid_exists', os.getpid())

    def test_virtual_memory(self):
        self.execute('virtual_memory')

    
    @unittest.skipIf(SUNOS,
                     "not worth being tested on SUNOS (uses a subprocess)")
    def test_swap_memory(self):
        self.execute('swap_memory')

    @skip_if_linux()
    def test_cpu_times(self):
        self.execute('cpu_times')

    @skip_if_linux()
    def test_per_cpu_times(self):
        self.execute('cpu_times', percpu=True)

    @unittest.skipIf(POSIX and SKIP_PYTHON_IMPL,
                     "not worth being tested on POSIX (pure python)")
    def test_disk_usage(self):
        self.execute('disk_usage', '.')

    def test_disk_partitions(self):
        self.execute('disk_partitions')

    @skip_if_linux()
    def test_net_io_counters(self):
        self.execute('net_io_counters')

    @unittest.skipIf(LINUX and not os.path.exists('/proc/diskstats'),
                     '/proc/diskstats not available on this Linux version')
    @skip_if_linux()
    def test_disk_io_counters(self):
        self.execute('disk_io_counters')

    
    @unittest.skipIf(WINDOWS, "XXX produces a false positive on Windows")
    def test_users(self):
        self.execute('users')

    @unittest.skipIf(LINUX,
                     "not worth being tested on Linux (pure python)")
    def test_net_connections(self):
        self.execute('net_connections')


def test_main():
    test_suite = unittest.TestSuite()
    tests = [TestProcessObjectLeaksZombie,
             TestProcessObjectLeaks,
             TestModuleFunctionsLeaks]
    for test in tests:
        test_suite.addTest(unittest.makeSuite(test))
    result = unittest.TextTestRunner(verbosity=2).run(test_suite)
    return result.wasSuccessful()

if __name__ == '__main__':
    if not test_main():
        sys.exit(1)
