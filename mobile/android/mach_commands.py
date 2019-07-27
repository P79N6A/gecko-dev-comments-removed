



from __future__ import print_function, unicode_literals

import argparse
import logging
import mozpack.path as mozpath

from mozbuild.base import (
    MachCommandBase,
    MachCommandConditions as conditions,
)

from mach.decorators import (
    CommandArgument,
    CommandProvider,
    Command,
)


@CommandProvider
class MachCommands(MachCommandBase):
    @Command('gradle', category='devenv',
        description='Run gradle.',
        conditions=[conditions.is_android])
    @CommandArgument('args', nargs=argparse.REMAINDER)
    def gradle(self, args):
        
        self.log_manager.terminal_handler.setLevel(logging.CRITICAL)

        return self.run_process(['./gradlew'] + args,
            pass_thru=True, 
            ensure_exit_code=False, 
            cwd=mozpath.join(self.topobjdir, 'mobile', 'android', 'gradle'))
