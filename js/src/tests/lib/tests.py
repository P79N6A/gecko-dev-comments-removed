




import datetime, os, sys, time
from subprocess import Popen, PIPE
from threading import Thread

from results import TestOutput



TBPL_FLAGS = [
    [], 
    ['--ion-eager', '--ion-offthread-compile=off'], 
    ['--ion-eager', '--ion-offthread-compile=off',
     '--ion-check-range-analysis', '--no-sse3', '--no-threads'],
    ['--baseline-eager'],
    ['--baseline-eager', '--no-fpu'],
    ['--no-baseline', '--no-ion'],
]

def do_run_cmd(cmd):
    l = [None, None]
    th_run_cmd(cmd, l)
    return l[1]

def set_limits():
    
    try:
        import resource
        GB = 2**30
        resource.setrlimit(resource.RLIMIT_AS, (2*GB, 2*GB))
    except:
        return

def th_run_cmd(cmd, l):
    t0 = datetime.datetime.now()

    
    
    options = {}
    if sys.platform != 'win32':
        options["close_fds"] = True
        options["preexec_fn"] = set_limits
    p = Popen(cmd, stdin=PIPE, stdout=PIPE, stderr=PIPE, **options)

    l[0] = p
    out, err = p.communicate()
    t1 = datetime.datetime.now()
    dd = t1-t0
    dt = dd.seconds + 1e-6 * dd.microseconds
    l[1] = (out, err, p.returncode, dt)

def run_cmd(cmd, timeout=60.0):
    if timeout is None:
        return do_run_cmd(cmd)

    l = [None, None]
    timed_out = False
    th = Thread(target=th_run_cmd, args=(cmd, l))
    th.start()
    th.join(timeout)
    while th.isAlive():
        if l[0] is not None:
            try:
                
                import signal
                if sys.platform != 'win32':
                    os.kill(l[0].pid, signal.SIGKILL)
                time.sleep(.1)
                timed_out = True
            except OSError:
                
                pass
    th.join()
    return l[1] + (timed_out,)

class Test(object):
    """A runnable test."""
    def __init__(self, path):
        self.path = path     
        self.options = []    
        self.jitflags = []   

    @staticmethod
    def prefix_command(path):
        """Return the '-f shell.js' options needed to run a test with the given
        path."""
        if path == '':
            return ['-f', 'shell.js']
        head, base = os.path.split(path)
        return Test.prefix_command(head) \
            + ['-f', os.path.join(path, 'shell.js')]

    def get_command(self, js_cmd_prefix):
        dirname, filename = os.path.split(self.path)
        cmd = js_cmd_prefix + self.jitflags + self.options \
              + Test.prefix_command(dirname) + ['-f', self.path]
        return cmd

    def run(self, js_cmd_prefix, timeout=30.0):
        cmd = self.get_command(js_cmd_prefix)
        out, err, rc, dt, timed_out = run_cmd(cmd, timeout)
        return TestOutput(self, cmd, out, err, rc, dt, timed_out)

class TestCase(Test):
    """A test case consisting of a test and an expected result."""
    js_cmd_prefix = None

    def __init__(self, path):
        Test.__init__(self, path)
        self.enable = True   
        self.expect = True   
        self.random = False  
        self.slow = False    

        
        self.terms = None

        
        self.tag = None

        
        self.comment = None

    def __str__(self):
        ans = self.path
        if not self.enable:
            ans += ', skip'
        if not self.expect:
            ans += ', fails'
        if self.random:
            ans += ', random'
        if self.slow:
            ans += ', slow'
        if '-d' in self.options:
            ans += ', debugMode'
        return ans

    @classmethod
    def set_js_cmd_prefix(self, js_path, js_args, debugger_prefix):
        parts = []
        if debugger_prefix:
            parts += debugger_prefix
        parts.append(js_path)
        if js_args:
            parts += js_args
        self.js_cmd_prefix = parts

    def __cmp__(self, other):
        if self.path == other.path:
            return 0
        elif self.path < other.path:
            return -1
        return 1

    def __hash__(self):
        return self.path.__hash__()
