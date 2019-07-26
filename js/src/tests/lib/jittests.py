







from __future__ import print_function
import os, posixpath, sys, tempfile, traceback, time
import subprocess
from subprocess import Popen, PIPE
from threading import Thread
import signal
import StringIO

try:
    from multiprocessing import Process, Manager, cpu_count
    HAVE_MULTIPROCESSING = True
except ImportError:
    HAVE_MULTIPROCESSING = False

from progressbar import ProgressBar, NullProgressBar
from results import TestOutput

TESTS_LIB_DIR = os.path.dirname(os.path.abspath(__file__))
JS_DIR = os.path.dirname(os.path.dirname(TESTS_LIB_DIR))
TEST_DIR = os.path.join(JS_DIR, 'jit-test', 'tests')
LIB_DIR = os.path.join(JS_DIR, 'jit-test', 'lib') + os.path.sep


def _relpath(path, start=None):
    """Return a relative version of a path"""

    if not path:
        raise ValueError("no path specified")

    if start is None:
        start = os.curdir

    start_list = os.path.abspath(start).split(os.sep)
    path_list = os.path.abspath(path).split(os.sep)

    
    i = len(os.path.commonprefix([start_list, path_list]))

    rel_list = [os.pardir] * (len(start_list)-i) + path_list[i:]
    if not rel_list:
        return os.curdir
    return os.path.join(*rel_list)

os.path.relpath = _relpath

class Test:

    VALGRIND_CMD = []
    paths = (d for d in os.environ['PATH'].split(os.pathsep))
    valgrinds = (os.path.join(d, 'valgrind') for d in paths)
    if any(os.path.exists(p) for p in valgrinds):
        VALGRIND_CMD = [
            'valgrind', '-q', '--smc-check=all-non-file',
            '--error-exitcode=1', '--gen-suppressions=all',
            '--show-possibly-lost=no', '--leak-check=full',
        ]
        if os.uname()[0] == 'Darwin':
            VALGRIND_CMD.append('--dsymutil=yes')

    del paths
    del valgrinds

    def __init__(self, path):
        self.path = path       

        self.jitflags = []     
        self.slow = False      
        self.allow_oom = False 
        self.valgrind = False  
        self.tz_pacific = False 
        self.expect_error = '' 
        self.expect_status = 0 

    def copy(self):
        t = Test(self.path)
        t.jitflags = self.jitflags[:]
        t.slow = self.slow
        t.allow_oom = self.allow_oom
        t.valgrind = self.valgrind
        t.tz_pacific = self.tz_pacific
        t.expect_error = self.expect_error
        t.expect_status = self.expect_status
        return t

    COOKIE = '|jit-test|'

    @classmethod
    def from_file(cls, path, options):
        test = cls(path)

        line = open(path).readline()
        i = line.find(cls.COOKIE)
        if i != -1:
            meta = line[i + len(cls.COOKIE):].strip('\n')
            parts = meta.split(';')
            for part in parts:
                part = part.strip()
                if not part:
                    continue
                name, _, value = part.partition(':')
                if value:
                    value = value.strip()
                    if name == 'error':
                        test.expect_error = value
                    elif name == 'exitstatus':
                        try:
                            test.expect_status = int(value, 0);
                        except ValueError:
                            print("warning: couldn't parse exit status %s" % value)
                    elif name == 'thread-count':
                        try:
                            test.jitflags.append('--thread-count=' + int(value, 0));
                        except ValueError:
                            print("warning: couldn't parse thread-count %s" % value)
                    else:
                        print('warning: unrecognized |jit-test| attribute %s' % part)
                else:
                    if name == 'slow':
                        test.slow = True
                    elif name == 'allow-oom':
                        test.allow_oom = True
                    elif name == 'valgrind':
                        test.valgrind = options.valgrind
                    elif name == 'tz-pacific':
                        test.tz_pacific = True
                    elif name == 'mjitalways':
                        test.jitflags.append('--always-mjit')
                    elif name == 'debug':
                        test.jitflags.append('--debugjit')
                    elif name == 'mjit':
                        test.jitflags.append('--jm')
                    elif name == 'no-jm':
                        test.jitflags.append('--no-jm')
                    elif name == 'ion-eager':
                        test.jitflags.append('--ion-eager')
                    elif name == 'no-ion':
                        test.jitflags.append('--no-ion')
                    elif name == 'dump-bytecode':
                        test.jitflags.append('--dump-bytecode')
                    else:
                        print('warning: unrecognized |jit-test| attribute %s' % part)

        if options.valgrind_all:
            test.valgrind = True

        return test

    def command(self, prefix, libdir, remote_prefix=None):
        path = self.path
        if remote_prefix:
            path = self.path.replace(TEST_DIR, remote_prefix)

        scriptdir_var = os.path.dirname(path);
        if not scriptdir_var.endswith('/'):
            scriptdir_var += '/'

        
        
        
        
        
        
        fmt = 'const platform=%r; const libdir=%r; const scriptdir=%r'
        if remote_prefix:
            fmt = 'const platform="%s"; const libdir="%s"; const scriptdir="%s"'
        expr = fmt % (sys.platform, libdir, scriptdir_var)

        
        
        cmd = prefix + list(set(self.jitflags)) + ['-e', expr, '-f', path]
        if self.valgrind:
            cmd = self.VALGRIND_CMD + cmd
        return cmd

def find_tests(substring=None):
    ans = []
    for dirpath, dirnames, filenames in os.walk(TEST_DIR):
        dirnames.sort()
        filenames.sort()
        if dirpath == '.':
            continue
        for filename in filenames:
            if not filename.endswith('.js'):
                continue
            if filename in ('shell.js', 'browser.js', 'jsref.js'):
                continue
            test = os.path.join(dirpath, filename)
            if substring is None or substring in os.path.relpath(test, TEST_DIR):
                ans.append(test)
    return ans

def tmppath(token):
    fd, path = tempfile.mkstemp(prefix=token)
    os.close(fd)
    return path

def read_and_unlink(path):
    f = open(path)
    d = f.read()
    f.close()
    os.unlink(path)
    return d

def th_run_cmd(cmdline, options, l):
    
    if sys.platform != 'win32':
        options["close_fds"] = True
    p = Popen(cmdline, stdin=PIPE, stdout=PIPE, stderr=PIPE, **options)

    l[0] = p
    out, err = p.communicate()
    l[1] = (out, err, p.returncode)

def run_timeout_cmd(cmdline, options, timeout=60.0):
    l = [ None, None ]
    timed_out = False
    th = Thread(target=th_run_cmd, args=(cmdline, options, l))

    
    
    
    
    sigint_handler = signal.getsignal(signal.SIGINT)
    sigterm_handler = signal.getsignal(signal.SIGTERM)
    if (sigint_handler == signal.SIG_IGN):
        def handleChildSignal(sig, frame):
            try:
                if sys.platform != 'win32':
                    os.kill(l[0].pid, signal.SIGKILL)
                else:
                    import ctypes
                    ctypes.windll.kernel32.TerminateProcess(int(l[0]._handle), -1)
            except OSError:
                pass
            if (sig == signal.SIGTERM):
                sys.exit(0)
        signal.signal(signal.SIGINT, handleChildSignal)
        signal.signal(signal.SIGTERM, handleChildSignal)

    th.start()
    th.join(timeout)
    while th.isAlive():
        if l[0] is not None:
            try:
                
                if sys.platform != 'win32':
                    os.kill(l[0].pid, signal.SIGKILL)
                else:
                    import ctypes
                    ctypes.windll.kernel32.TerminateProcess(int(l[0]._handle), -1)
                time.sleep(.1)
                timed_out = True
            except OSError:
                
                pass
    th.join()

    
    if (sigint_handler == signal.SIG_IGN):
        signal.signal(signal.SIGINT, signal.SIG_IGN)
        signal.signal(signal.SIGTERM, sigterm_handler)

    (out, err, code) = l[1]

    return (out, err, code, timed_out)

def run_cmd(cmdline, env, timeout):
    return run_timeout_cmd(cmdline, { 'env': env }, timeout)

def run_cmd_avoid_stdio(cmdline, env, timeout):
    stdoutPath, stderrPath = tmppath('jsstdout'), tmppath('jsstderr')
    env['JS_STDOUT'] = stdoutPath
    env['JS_STDERR'] = stderrPath
    _, __, code = run_timeout_cmd(cmdline, { 'env': env }, timeout)
    return read_and_unlink(stdoutPath), read_and_unlink(stderrPath), code

def run_test(test, prefix, options):
    cmd = test.command(prefix, LIB_DIR)
    if options.show_cmd:
        print(subprocess.list2cmdline(cmd))

    if options.avoid_stdio:
        run = run_cmd_avoid_stdio
    else:
        run = run_cmd

    env = os.environ.copy()
    if test.tz_pacific:
        env['TZ'] = 'PST8PDT'

    out, err, code, timed_out = run(cmd, env, options.timeout)
    return TestOutput(test, cmd, out, err, code, None, timed_out)

def run_test_remote(test, device, prefix, options):
    cmd = test.command(prefix, posixpath.join(options.remote_test_root, 'lib/'), posixpath.join(options.remote_test_root, 'tests'))
    if options.show_cmd:
        print(subprocess.list2cmdline(cmd))

    env = {}
    if test.tz_pacific:
        env['TZ'] = 'PST8PDT'

    env['LD_LIBRARY_PATH'] = options.remote_test_root

    buf = StringIO.StringIO()
    returncode = device.shell(cmd, buf, env=env, cwd=options.remote_test_root,
                              timeout=int(options.timeout))

    out = buf.getvalue()
    
    
    return TestOutput(test, cmd, out, out, returncode, None, False)

def check_output(out, err, rc, test):
    if test.expect_error:
        
        return test.expect_error in err and rc == 3

    for line in out.split('\n'):
        if line.startswith('Trace stats check failed'):
            return False

    for line in err.split('\n'):
        if 'Assertion failed:' in line:
            return False

    if rc != test.expect_status:
        
        
        return test.allow_oom and 'out of memory' in err and 'Assertion failure' not in err

    return True

def print_tinderbox(label, test, message=None):
    
    
    
    
    if (test != None):
        jitflags = " ".join(test.jitflags)
        result = "%s | %s | %s" % (label, test.path, jitflags)
    else:
        result = "%s | jit_test.py" % label

    if message:
        result += ": " + message
    print(result)

def wrap_parallel_run_test(test, prefix, resultQueue, options):
    
    signal.signal(signal.SIGINT, signal.SIG_IGN)
    result = run_test(test, prefix, options)
    resultQueue.put(result)
    return result

def run_tests_parallel(tests, prefix, options):
    
    
    
    queue_manager = Manager()
    async_test_result_queue = queue_manager.Queue()

    
    
    
    
    notify_queue = queue_manager.Queue()

    
    
    result_process_return_queue = queue_manager.Queue()
    result_process = Process(target=process_test_results_parallel,
                             args=(async_test_result_queue, result_process_return_queue,
                                   notify_queue, len(tests), options))
    result_process.start()

    
    
    sigint_handler = signal.getsignal(signal.SIGINT)
    signal.signal(signal.SIGTERM, sigint_handler)

    worker_processes = []

    def remove_completed_workers(workers):
        new_workers = []
        for worker in workers:
            if worker.is_alive():
                new_workers.append(worker)
            else:
                worker.join()
        return new_workers

    try:
        testcnt = 0
        
        for i in range(min(options.max_jobs,len(tests))):
            notify_queue.put(True)

        
        
        while notify_queue.get():
            if (testcnt < len(tests)):
                
                worker_process = Process(target=wrap_parallel_run_test, args=(tests[testcnt], prefix, async_test_result_queue, options))
                worker_processes.append(worker_process)
                worker_process.start()
                testcnt += 1

                
                worker_processes = remove_completed_workers(worker_processes)
            else:
                break

        
        while len(worker_processes) > 0:
            worker_processes = remove_completed_workers(worker_processes)

        
        async_test_result_queue.put(None)
        result_process.join()

        
        return result_process_return_queue.get()
    except (Exception, KeyboardInterrupt) as e:
        
        
        if not isinstance(e,KeyboardInterrupt):
            traceback.print_exc()

        for worker in worker_processes:
            try:
                worker.terminate()
            except:
                pass

        result_process.terminate()

    return False

def get_parallel_results(async_test_result_queue, notify_queue):
    while True:
        async_test_result = async_test_result_queue.get()

        
        if (async_test_result == None):
            return

        
        notify_queue.put(True)

        yield async_test_result

def process_test_results_parallel(async_test_result_queue, return_queue, notify_queue, num_tests, options):
    gen = get_parallel_results(async_test_result_queue, notify_queue)
    ok = process_test_results(gen, num_tests, options)
    return_queue.put(ok)

def print_test_summary(failures, complete, doing, options):
    if failures:
        if options.write_failures:
            try:
                out = open(options.write_failures, 'w')
                
                written = set()
                for res in failures:
                    if res.test.path not in written:
                        out.write(os.path.relpath(res.test.path, TEST_DIR) + '\n')
                        if options.write_failure_output:
                            out.write(res.out)
                            out.write(res.err)
                            out.write('Exit code: ' + str(res.rc) + "\n")
                        written.add(res.test.path)
                out.close()
            except IOError:
                sys.stderr.write("Exception thrown trying to write failure file '%s'\n"%
                                 options.write_failures)
                traceback.print_exc()
                sys.stderr.write('---\n')

        def show_test(res):
            if options.show_failed:
                print('    ' + subprocess.list2cmdline(res.cmd))
            else:
                print('    ' + ' '.join(res.test.jitflags + [res.test.path]))

        print('FAILURES:')
        for res in failures:
            if not res.timed_out:
                show_test(res)

        print('TIMEOUTS:')
        for res in failures:
            if res.timed_out:
                show_test(res)

        return False
    else:
        print('PASSED ALL' + ('' if complete else ' (partial run -- interrupted by user %s)' % doing))
        return True

def process_test_results(results, num_tests, options):
    pb = NullProgressBar()
    if not options.hide_progress and not options.show_cmd and ProgressBar.conservative_isatty():
        fmt = [
            {'value': 'PASS',    'color': 'green'},
            {'value': 'FAIL',    'color': 'red'},
            {'value': 'TIMEOUT', 'color': 'blue'},
            {'value': 'SKIP',    'color': 'brightgray'},
        ]
        pb = ProgressBar(num_tests, fmt)

    failures = []
    timeouts = 0
    complete = False
    doing = 'before starting'
    try:
        for i, res in enumerate(results):

            if options.show_output:
                sys.stdout.write(res.out)
                sys.stdout.write(res.err)
                sys.stdout.write('Exit code: %s\n' % res.rc)
            if res.test.valgrind:
                sys.stdout.write(res.err)

            ok = check_output(res.out, res.err, res.rc, res.test)
            doing = 'after %s' % res.test.path
            if not ok:
                failures.append(res)
                if res.timed_out:
                    pb.message("TIMEOUT - %s" % res.test.path)
                    timeouts += 1
                else:
                    pb.message("FAIL - %s" % res.test.path)

            if options.tinderbox:
                if ok:
                    print_tinderbox("TEST-PASS", res.test);
                else:
                    lines = [ _ for _ in res.out.split('\n') + res.err.split('\n')
                              if _ != '' ]
                    if len(lines) >= 1:
                        msg = lines[-1]
                    else:
                        msg = ''
                    print_tinderbox("TEST-UNEXPECTED-FAIL", res.test, msg);

            n = i + 1
            pb.update(n, {
                'PASS': n - len(failures),
                'FAIL': len(failures),
                'TIMEOUT': timeouts,
                'SKIP': 0}
            )
        complete = True
    except KeyboardInterrupt:
        print_tinderbox("TEST-UNEXPECTED-FAIL", None, "Test execution interrupted by user");

    pb.finish(True)
    return print_test_summary(failures, complete, doing, options)

def get_serial_results(tests, prefix, options):
    for test in tests:
        yield run_test(test, prefix, options)

def run_tests(tests, prefix, options):
    gen = get_serial_results(tests, prefix, options)
    ok = process_test_results(gen, len(tests), options)
    return ok

def get_remote_results(tests, device, prefix, options):
    for test in tests:
        yield run_test_remote(test, device, prefix, options)

def push_libs(options, device):
    
    
    required_libs = ['libnss3.so', 'libmozglue.so']

    for file in os.listdir(options.local_lib):
        if file in required_libs:
            remote_file = posixpath.join(options.remote_test_root, file)
            device.pushFile(os.path.join(options.local_lib, file), remote_file)

def push_progs(options, device, progs):
    for local_file in progs:
        remote_file = posixpath.join(options.remote_test_root, os.path.basename(local_file))
        device.pushFile(local_file, remote_file)

def run_tests_remote(tests, prefix, options):
    
    from mozdevice import devicemanager, devicemanagerADB, devicemanagerSUT

    if options.device_transport == 'adb':
        if options.device_ip:
            dm = devicemanagerADB.DeviceManagerADB(options.device_ip, options.devicePort, packageName=None, deviceRoot=options.remoteTestRoot)
        else:
            dm = devicemanagerADB.DeviceManagerADB(packageName=None, deviceRoot=options.remote_test_root)
    else:
        dm = devicemanagerSUT.DeviceManagerSUT(options.device_ip, options.device_port, deviceRoot=options.remote_test_root)
        if options.device_ip == None:
            print('Error: you must provide a device IP to connect to via the --device option')
            sys.exit(1)

    
    options.remote_test_root = posixpath.join(options.remote_test_root, 'jit-tests')

    
    if dm.dirExists(options.remote_test_root):
        dm.removeDir(options.remote_test_root)
    dm.mkDir(options.remote_test_root)
    push_libs(options, dm)
    push_progs(options, dm, [prefix[0]])
    dm.chmodDir(options.remote_test_root)
    dm.pushDir(os.path.dirname(TEST_DIR), options.remote_test_root)
    prefix[0] = os.path.join(options.remote_test_root, 'js')

    
    gen = get_remote_results(tests, dm, prefix, options)
    ok = process_test_results(gen, len(tests), options)
    return ok

def parse_jitflags(options):
    jitflags = [ [ '-' + flag for flag in flags ]
                 for flags in options.jitflags.split(',') ]
    for flags in jitflags:
        for flag in flags:
            if flag not in ('-m', '-a', '-p', '-d', '-n'):
                print('Invalid jit flag: "%s"' % flag)
                sys.exit(1)
    return jitflags

def platform_might_be_android():
    try:
        
        
        
        
        import android
        return True
    except ImportError:
        return False

def stdio_might_be_broken():
    return platform_might_be_android()

if __name__ == '__main__':
    print('Use ../jit-test/jit_test.py to run these tests.')
