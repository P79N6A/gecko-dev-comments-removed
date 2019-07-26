



from __future__ import print_function, unicode_literals

import os
import sys

from mach.decorators import (
    CommandProvider,
    Command,
)


@CommandProvider
class VersionControlCommands(object):
    def __init__(self, context):
        self._context = context

    @Command('mercurial-setup', category='devenv',
        description='Help configure Mercurial for optimal development.')
    def mercurial_bootstrap(self):
        sys.path.append(os.path.dirname(__file__))

        from hgsetup.wizard import MercurialSetupWizard

        wizard = MercurialSetupWizard(self._context.state_dir)
        config_paths = ['~/.hgrc']
        if sys.platform in ('win32', 'cygwin'):
          config_paths.insert(0, '~/mercurial.ini')
        result = wizard.run(map(os.path.expanduser, config_paths))

        
        state_path = os.path.join(self._context.state_dir,
            'mercurial/setup.lastcheck')
        with open(state_path, 'a'):
            os.utime(state_path, None)

        return result
