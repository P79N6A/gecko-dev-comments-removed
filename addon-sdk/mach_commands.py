





import os
import sys

from mozbuild.base import (
    MachCommandBase,
    MozbuildObject,
)

from mach.decorators import (
    CommandArgument,
    CommandProvider,
    Command,
)

class JetpackRunner(MozbuildObject):
    """Run jetpack tests."""
    def run_tests(self, **kwargs):
        self._run_make(target='jetpack-tests')

@CommandProvider
class MachCommands(MachCommandBase):
    @Command('jetpack-test', category='testing',
        description='Runs the jetpack test suite (Add-on SDK).')
    def run_jetpack_test(self, **params):
        
        
        
        self._ensure_state_subdir_exists('.')

        jetpack = self._spawn(JetpackRunner)

        jetpack.run_tests(**params)
