



from __future__ import unicode_literals

import difflib
import errno
import os
import shutil
import sys
import which

from configobj import ConfigObjError
from StringIO import StringIO

from mozversioncontrol.repoupdate import (
    update_mercurial_repo,
    update_git_repo,
)

from .config import (
    HOST_FINGERPRINTS,
    MercurialConfig,
)


INITIAL_MESSAGE = '''
I'm going to help you ensure your Mercurial is configured for optimal
development on Mozilla projects.

If your environment is missing some recommended settings, I'm going to prompt
you whether you want me to make changes: I won't change anything you might not
want me changing without your permission!

If your config is up-to-date, I'm just going to ensure all 3rd party extensions
are up to date and you won't have to do anything.

To begin, press the enter/return key.
'''.strip()

MISSING_USERNAME = '''
You don't have a username defined in your Mercurial config file. In order to
send patches to Mozilla, you'll need to attach a name and email address. If you
aren't comfortable giving us your full name, pseudonames are acceptable.
'''.strip()

BAD_DIFF_SETTINGS = '''
Mozilla developers produce patches in a standard format, but your Mercurial is
not configured to produce patches in that format.
'''.strip()

BZEXPORT_INFO = '''
If you plan on uploading patches to Mozilla, there is an extension called
bzexport that makes it easy to upload patches from the command line via the
|hg bzexport| command. More info is available at
https://hg.mozilla.org/hgcustom/version-control-tools/file/default/hgext/bzexport/README
'''.strip()

MQEXT_INFO = '''
The mqext extension (https://bitbucket.org/sfink/mqext) provides a number of
useful abilities to Mercurial, including automatically committing changes to
your mq patch queue.
'''.strip()

QIMPORTBZ_INFO = '''
The qimportbz extension
(https://hg.mozilla.org/hgcustom/version-control-tools/file/default/hgext/qimportbz/README) makes it possible to
import patches from Bugzilla using a friendly bz:// URL handler. e.g.
|hg qimport bz://123456|.
'''.strip()

QNEWCURRENTUSER_INFO = '''
The mercurial queues command |hg qnew|, which creates new patches in your patch
queue does not set patch author information by default. Author information
should be included when uploading for review.
'''.strip()

FINISHED = '''
Your Mercurial should now be properly configured and recommended extensions
should be up to date!
'''.strip()


class MercurialSetupWizard(object):
    """Command-line wizard to help users configure Mercurial."""

    def __init__(self, state_dir):
        self.state_dir = state_dir
        self.ext_dir = os.path.join(state_dir, 'mercurial', 'extensions')
        self.vcs_tools_dir = os.path.join(state_dir, 'version-control-tools')

    def run(self, config_paths):
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

        try:
            c = MercurialConfig(config_paths)
        except ConfigObjError as e:
            print('Error importing existing Mercurial config!\n'
                  '%s\n'
                  'If using quotes, they must wrap the entire string.' % e)
            return 1

        print(INITIAL_MESSAGE)
        raw_input()

        if not c.have_valid_username():
            print(MISSING_USERNAME)
            print('')

            name = self._prompt('What is your name?')
            email = self._prompt('What is your email address?')
            c.set_username(name, email)
            print('Updated your username.')
            print('')

        if not c.have_recommended_diff_settings():
            print(BAD_DIFF_SETTINGS)
            print('')
            if self._prompt_yn('Would you like me to fix this for you'):
                c.ensure_recommended_diff_settings()
                print('Fixed patch settings.')
                print('')

        active = c.extensions

        if 'progress' not in active:
            if self._prompt_yn('Would you like to see progress bars during '
                'long-running Mercurial operations'):
                c.activate_extension('progress')
                print('Activated progress extension.')
                print('')

        if 'color' not in active:
            if self._prompt_yn('Would you like Mercurial to colorize output '
                'to your terminal'):
                c.activate_extension('color')
                print('Activated color extension.')
                print('')

        if 'rebase' not in active:
            if self._prompt_yn('Would you like to enable the rebase extension '
                'to allow you to move changesets around (which can help '
                'maintain a linear history)'):
                c.activate_extension('rebase')
                print('Activated rebase extension.')
                print('')

        update_vcs_tools = False
        activate_bzexport = False

        if 'bzexport' not in active:
            print(BZEXPORT_INFO)
            if self._prompt_yn('Would you like to activate bzexport'):
                activate_bzexport = True
                update_vcs_tools = True
        else:
            activate_bzexport = True

        if activate_bzexport:
            update_vcs_tools = True
            c.activate_extension('bzexport',
                os.path.join(self.vcs_tools_dir, 'hgext', 'bzexport'))
            print('Activated bzexport extension.')
            print('')

        if 'mq' not in active:
            if self._prompt_yn('Would you like to activate the mq extension '
                'to manage patches'):
                c.activate_extension('mq')
                print('Activated mq extension.')
                print('')

        active = c.extensions

        if 'mq' in active:
            update_mqext = 'mqext' in active
            if 'mqext' not in active:
                print(MQEXT_INFO)
                if self._prompt_yn('Would you like to activate mqext and '
                    'automatically commit changes as you modify patches'):
                    update_mqext = True
                    c.activate_extension('mqext', os.path.join(self.ext_dir,
                        'mqext'))
                    c.autocommit_mq(True)
                    print('Activated mqext extension.')
                    print('')

            if update_mqext:
                self.update_mercurial_repo(
                hg,
                'https://bitbucket.org/sfink/mqext',
                os.path.join(self.ext_dir, 'mqext'),
                'default',
                'Ensuring mqext extension is up to date...')

            activate_qimportbz = True
            if 'qimportbz' not in active:
                print(QIMPORTBZ_INFO)
                if not self._prompt_yn('Would you like to activate qimportbz'):
                    activate_qimportbz = False

            if activate_qimportbz:
                update_vcs_tools = True
                c.activate_extension('qimportbz',
                    os.path.join(self.vcs_tools_dir, 'hgext', 'qimportbz'))
                print('Activated qimportbz extension.')
                print('')

            if not c.have_qnew_currentuser_default():
                print(QNEWCURRENTUSER_INFO)
                if self._prompt_yn('Would you like qnew to set patch author by '
                                   'default'):
                    c.ensure_qnew_currentuser_default()
                    print('Configured qnew to set patch author by default.')
                    print('')

        if update_vcs_tools:
            self.update_mercurial_repo(
                hg,
                'https://hg.mozilla.org/hgcustom/version-control-tools',
                self.vcs_tools_dir,
                'default',
                'Ensuring version-control-tools is up to date...')

        
        for ext in {'bzexport', 'qimportbz'}:
            path = os.path.join(self.ext_dir, ext)
            if os.path.exists(path):
                if self._prompt_yn('Would you like to remove the old and no '
                    'longer referenced repository at %s' % path):
                    print('Cleaning up old repository: %s' % path)
                    shutil.rmtree(path)

        c.add_mozilla_host_fingerprints()

        b = StringIO()
        c.write(b)
        new_lines = [line.rstrip() for line in b.getvalue().splitlines()]
        old_lines = []

        config_path = c.config_path
        if os.path.exists(config_path):
            with open(config_path, 'rt') as fh:
                old_lines = [line.rstrip() for line in fh.readlines()]

        diff = list(difflib.unified_diff(old_lines, new_lines,
            'hgrc.old', 'hgrc.new'))

        if len(diff):
            print('Your Mercurial config file needs updating. I can do this '
                'for you if you like!')
            if self._prompt_yn('Would you like to see a diff of the changes '
                'first'):
                for line in diff:
                    print(line)
                print('')

            if self._prompt_yn('Would you like me to update your hgrc file'):
                with open(config_path, 'wt') as fh:
                    c.write(fh)
                print('Wrote changes to %s.' % config_path)
            else:
                print('hgrc changes not written to file. I would have '
                    'written the following:\n')
                c.write(sys.stdout)
                return 1

        print(FINISHED)
        return 0

    def update_mercurial_repo(self, hg, url, dest, branch, msg):
        
        
        
        return self._update_repo(hg, url, dest, branch, msg,
            update_mercurial_repo, hostfingerprints=HOST_FINGERPRINTS)

    def update_git_repo(self, git, url, dest, ref, msg):
        return self._update_repo(git, url, dest, ref, msg, update_git_repo)

    def _update_repo(self, binary, url, dest, branch, msg, fn, *args, **kwargs):
        print('=' * 80)
        print(msg)
        try:
            fn(binary, url, dest, branch, *args, **kwargs)
        finally:
            print('=' * 80)
            print('')

    def _prompt(self, msg):
        print(msg)

        while True:
            response = raw_input()

            if response:
                return response

            print('You must type something!')

    def _prompt_yn(self, msg):
        print('%s? [Y/n]' % msg)

        while True:
            choice = raw_input().lower().strip()

            if not choice:
                return True

            if choice in ('y', 'yes'):
                return True

            if choice in ('n', 'no'):
                return False

            print('Must reply with one of {yes, no, y, no}.')
