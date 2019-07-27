



from __future__ import absolute_import, print_function, unicode_literals

import argparse
import logging
import mozpack.path as mozpath
import os
import which

from mozbuild.base import (
    MachCommandBase,
)

from mach.decorators import (
    CommandArgument,
    CommandProvider,
    Command,
)


ESLINT_NOT_FOUND_MESSAGE = '''
Could not find eslint!  We looked at the --binary option, at the ESLINT
environment variable, and then at your path.  Install eslint and needed plugins
with

npm install -g eslint eslint-plugin-react

and try again.
'''.strip()


@CommandProvider
class MachCommands(MachCommandBase):
    @Command('python', category='devenv',
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
        import glob

        
        
        
        
        
        return_code = 0
        files = []
        
        
        
        
        
        search_dirs = ['.', self.topsrcdir]
        last_search_dir = search_dirs[-1]
        for t in tests:
            for d in search_dirs:
                test = mozpath.join(d, t)
                if test.endswith('.py') and os.path.isfile(test):
                    files.append(test)
                    break
                elif os.path.isfile(test + '.py'):
                    files.append(test + '.py')
                    break
                elif os.path.isdir(test):
                    files += glob.glob(mozpath.join(test, 'test*.py'))
                    files += glob.glob(mozpath.join(test, 'unit*.py'))
                    break
                elif d == last_search_dir:
                    self.log(logging.WARN, 'python-test',
                             {'test': t},
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

    @Command('eslint', category='devenv')
    @CommandArgument('path', nargs='?', default='.',
        help='Path to files to lint, like "browser/components/loop" '
            'or "mobile/android". '
            'Defaults to the current directory if not given.')
    @CommandArgument('-e', '--ext', default='[.js,.jsm,.jsx]',
        help='Filename extensions to lint, default: "[.js,.jsm,.jsx]".')
    @CommandArgument('-b', '--binary', default=None,
        help='Path to eslint binary.')
    @CommandArgument('args', nargs=argparse.REMAINDER)  
    def eslint(self, path, ext=None, binary=None, args=[]):
        '''Run eslint.'''

        if not binary:
            binary = os.environ.get('ESLINT', None)
            if not binary:
                try:
                    binary = which.which('eslint')
                except which.WhichError:
                    pass

        if not binary:
            print(ESLINT_NOT_FOUND_MESSAGE)
            return 1

        
        
        
        
        
        
        
        
        
        

        self.log(logging.INFO, 'eslint', {'binary': binary, 'path': path},
            'Running {binary} in {path}')

        cmd_args = [binary,
            '--ext', ext,  
        ] + args
        
        cmd_args += ['.']

        return self.run_process(cmd_args,
            cwd=path,
            pass_thru=True,  
            ensure_exit_code=False,  
        )
