



from __future__ import unicode_literals

import os
import sys

from mach.decorators import (
    CommandArgument,
    CommandProvider,
    Command,
)

from mozbuild.base import MachCommandBase


@CommandProvider
class WebIDLProvider(MachCommandBase):
    @Command('webidl-parser-test', category='testing',
        description='Run WebIDL tests.')
    @CommandArgument('--verbose', '-v', action='store_true',
        help='Run tests in verbose mode.')
    def webidl_test(self, verbose=False):
        sys.path.insert(0, os.path.join(self.topsrcdir, 'other-licenses',
            'ply'))

        from runtests import run_tests
        return run_tests(None, verbose=verbose)
