




import datetime, os, sys, time
from subprocess import Popen, PIPE
from threading import Thread

from results import TestOutput



JITFLAGS = {
    'all': [
        [], 
        ['--ion-eager', '--ion-offthread-compile=off'], 
        ['--ion-eager', '--ion-offthread-compile=off', '--non-writable-jitcode',
         '--ion-check-range-analysis', '--ion-extra-checks', '--no-sse3', '--no-threads'],
        ['--baseline-eager'],
        ['--baseline-eager', '--no-fpu'],
        ['--no-baseline', '--no-ion'],
    ],
    
    'ion': [
        ['--baseline-eager'],
        ['--ion-eager', '--ion-offthread-compile=off']
    ],
    
    'debug': [
        [], 
        ['--ion-eager', '--ion-offthread-compile=off'], 
        ['--baseline-eager'],
    ],
    'none': [
        [] 
    ]
}

def get_jitflags(variant, **kwargs):
    if variant not in JITFLAGS:
        print('Invalid jitflag: "{}"'.format(variant))
        sys.exit(1)
    if variant == 'none' and 'none' in kwargs:
        return kwargs['none']
    return JITFLAGS[variant]

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

    def get_command(self, prefix):
        dirname, filename = os.path.split(self.path)
        cmd = prefix + self.jitflags + self.options \
              + Test.prefix_command(dirname) + ['-f', self.path]
        return cmd

class TestCase(Test):
    """A test case consisting of a test and an expected result."""
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

    @staticmethod
    def build_js_cmd_prefix(js_path, js_args, debugger_prefix):
        parts = []
        if debugger_prefix:
            parts += debugger_prefix
        parts.append(js_path)
        if js_args:
            parts += js_args
        return parts

    def __cmp__(self, other):
        if self.path == other.path:
            return 0
        elif self.path < other.path:
            return -1
        return 1

    def __hash__(self):
        return self.path.__hash__()
