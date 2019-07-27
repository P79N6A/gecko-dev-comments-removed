



from __future__ import absolute_import, print_function, unicode_literals

import os
import sys

from mach.decorators import (
    CommandProvider,
    CommandArgument,
    Command,
)


@CommandProvider
class VersionControlCommands(object):
    def __init__(self, context):
        self._context = context

    @Command('mercurial-setup', category='devenv',
        description='Help configure Mercurial for optimal development.')
    @CommandArgument('-u', '--update-only', action='store_true',
        help='Only update recommended extensions, don\'t run the wizard.')
    def mercurial_bootstrap(self, update_only=False):
        sys.path.append(os.path.dirname(__file__))

        config_paths = ['~/.hgrc']
        if sys.platform in ('win32', 'cygwin'):
            config_paths.insert(0, '~/mercurial.ini')
        config_paths = map(os.path.expanduser, config_paths)

        
        
        
        
        
        state_path = os.path.join(self._context.state_dir,
            'mercurial/setup.lastcheck')
        with open(state_path, 'a'):
            os.utime(state_path, None)

        if update_only:
            from hgsetup.update import MercurialUpdater
            updater = MercurialUpdater(self._context.state_dir)
            result = updater.update_all(map(os.path.expanduser, config_paths))
        else:
            from hgsetup.wizard import MercurialSetupWizard
            wizard = MercurialSetupWizard(self._context.state_dir)
            result = wizard.run(map(os.path.expanduser, config_paths))

        return result
