



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
    @Command('python', category='devenv',
        allow_all_args=True,
        description='Run Python.')
    @CommandArgument('args', nargs=argparse.REMAINDER)
    def python(self, args):
        
        self.log_manager.terminal_handler.setLevel(logging.CRITICAL)

        self._activate_virtualenv()

        return self.run_process([self.virtualenv_manager.python_path] + args,
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
        self._activate_virtualenv()

        
        
        
        
        
        return_code = 0
        files = []
        for test in [mozpack.path.join(self.topsrcdir, t) for t in tests]:
            if test.endswith('.py') and os.path.isfile(test):
                files.append(test)
            elif os.path.isfile(test + '.py'):
                files.append(test + '.py')
            elif os.path.isdir(test):
                files += glob.glob(mozpack.path.join(test, 'test*.py'))
                files += glob.glob(mozpack.path.join(test, 'unit*.py'))
            else:
                self.log(logging.WARN, 'python-test',
                         {'test': mozpack.path.relpath(test, self.topsrcdir)},
                         'TEST-UNEXPECTED-FAIL | Invalid test: {test}')
                if stop:
                    return 1

        for f in files:
            file_displayed_test = [] 
            def _line_handler(line):
                if not file_displayed_test and line.startswith('TEST-'):
                    file_displayed_test.append(True)

            inner_return_code = self.run_process(
                [self.virtualenv_manager.python_path, f],
                ensure_exit_code=False, 
                log_name='python-test',
                
                append_env={b'PYTHONDONTWRITEBYTECODE': str('1')},
                line_handler=_line_handler)
            return_code += inner_return_code

            if not file_displayed_test:
                self.log(logging.WARN, 'python-test', {'file': f},
                         'TEST-UNEXPECTED-FAIL | No test output (missing mozunit.main() call?): {file}')

            if verbose:
                if inner_return_code != 0:
                    self.log(logging.INFO, 'python-test', {'file': f},
                             'Test failed: {file}')
                else:
                    self.log(logging.INFO, 'python-test', {'file': f},
                             'Test passed: {file}')
            if stop and return_code > 0:
                return 1

        return 0 if return_code == 0 else 1
