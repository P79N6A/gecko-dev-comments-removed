



from __future__ import unicode_literals

import errno
import os
import which

from mozversioncontrol.repoupdate import update_mercurial_repo

from .config import (
    HOST_FINGERPRINTS,
)

FINISHED = '''
Your Mercurial recommended extensions are now up to date!
'''.lstrip()


class MercurialUpdater(object):

    def __init__(self, state_dir):
        self.state_dir = os.path.normpath(state_dir)
        self.ext_dir = os.path.join(self.state_dir, 'mercurial', 'extensions')
        self.vcs_tools_dir = os.path.join(self.state_dir, 'version-control-tools')

    def update_all(self):
        try:
            os.makedirs(self.ext_dir)
        except OSError as e:
            if e.errno != errno.EEXIST:
                raise

        try:
            hg = which.which('hg')
        except which.WhichError as e:
            print(e)
            print('Try running |mach bootstrap| to ensure your environment is '
                'up to date.')
            return 1

        if os.path.isdir(self.vcs_tools_dir):
            self.update_mercurial_repo(
                hg,
                'https://hg.mozilla.org/hgcustom/version-control-tools',
                self.vcs_tools_dir,
                'default',
                'Ensuring version-control-tools is up to date...')
        print(FINISHED)
        return 0

    def update_mercurial_repo(self, hg, url, dest, branch, msg):
        
        
        
        return self._update_repo(hg, url, dest, branch, msg,
            update_mercurial_repo, hostfingerprints=HOST_FINGERPRINTS)

    def _update_repo(self, binary, url, dest, branch, msg, fn, *args, **kwargs):
        print('=' * 80)
        print(msg)
        try:
            fn(binary, url, dest, branch, *args, **kwargs)
        finally:
            print('=' * 80)
            print('')
