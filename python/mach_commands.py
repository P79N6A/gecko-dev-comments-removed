



from __future__ import print_function, unicode_literals

import argparse
import glob
import logging
import mozpack.path
import os
import sys

from mozbuild.base import (
    MachCommandBase,
)

from mach.decorators import (
    CommandArgument,
    CommandProvider,
    Command,
)


@CommandProvider
class MachCommands(MachCommandBase):
    '''
    Easily run Python and Python unit tests.
    '''
    def __init__(self, context):
        MachCommandBase.__init__(self, context)
        self._python_executable = None

    @property
    def python_executable(self):
        '''
        Return path to Python executable, or print and sys.exit(1) if
        executable does not exist.
        '''
        if self._python_executable:
            return self._python_executable
        if self._is_windows():
            executable = '_virtualenv/Scripts/python.exe'
        else:
            executable = '_virtualenv/bin/python'
        path = mozpack.path.join(self.topobjdir, executable)
        if not os.path.exists(path):
            print("Could not find Python executable at %s." % path,
                  "Run |mach configure| or |mach build| to install it.")
            sys.exit(1)
        self._python_executable = path
        return path

    @Command('python', category='devenv',
        allow_all_args=True,
        description='Run Python.')
    @CommandArgument('args', nargs=argparse.REMAINDER)
    def python(self, args):
        
        self.log_manager.terminal_handler.setLevel(logging.CRITICAL)
        return self.run_process([self.python_executable] + args,
            pass_thru=True, 
            ensure_exit_code=False, 
            
            append_env={b'PYTHONDONTWRITEBYTECODE': str('1')})

    @Command('python-test', category='testing',
        description='Run Python unit tests.')
    @CommandArgument('--verbose',
        default=False,
        action='store_true',
        help='Verbose output.')
    @CommandArgument('--stop',
        default=False,
        action='store_true',
        help='Stop running tests after the first error or failure.')
    @CommandArgument('tests', nargs='+',
        metavar='TEST',
        help='Tests to run. Each test can be a single file or a directory.')
    def python_test(self, tests, verbose=False, stop=False):
        
        self.python_executable

        
        
        
        
        
        return_code = 0
        files = []
        for test in tests:
            if test.endswith('.py') and os.path.isfile(test):
                files.append(test)
            elif os.path.isfile(test + '.py'):
                files.append(test + '.py')
            elif os.path.isdir(test):
                files += glob.glob(mozpack.path.join(test, 'test*.py'))
                files += glob.glob(mozpack.path.join(test, 'unit*.py'))
            else:
                self.log(logging.WARN, 'python-test', {'test': test},
                         'TEST-UNEXPECTED-FAIL | Invalid test: {test}')
                if stop:
                    return 1

        for file in files:
            file_displayed_test = [] 
            def _line_handler(line):
                if not file_displayed_test and line.startswith('TEST-'):
                    file_displayed_test.append(True)

            inner_return_code = self.run_process(
                [self.python_executable, file],
                ensure_exit_code=False, 
                log_name='python-test',
                
                append_env={b'PYTHONDONTWRITEBYTECODE': str('1')},
                line_handler=_line_handler)
            return_code += inner_return_code

            if not file_displayed_test:
                self.log(logging.WARN, 'python-test', {'file': file},
                         'TEST-UNEXPECTED-FAIL | No test output (missing mozunit.main() call?): {file}')

            if verbose:
                if inner_return_code != 0:
                    self.log(logging.INFO, 'python-test', {'file': file},
                             'Test failed: {file}')
                else:
                    self.log(logging.INFO, 'python-test', {'file': file},
                             'Test passed: {file}')
            if stop and return_code > 0:
                return 1

        return 0 if return_code == 0 else 1
