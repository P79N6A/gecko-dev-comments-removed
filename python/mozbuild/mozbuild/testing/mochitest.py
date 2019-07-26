



from __future__ import unicode_literals

import os

from mozbuild.base import MozbuildObject


class MochitestRunner(MozbuildObject):
    """Easily run mochitests.

    This currently contains just the basics for running mochitests. We may want
    to hook up result parsing, etc.
    """
    def run_plain_suite(self):
        """Runs all plain mochitests."""
        
        self._run_make(directory='.', target='mochitest-plain')

    def run_chrome_suite(self):
        """Runs all chrome mochitests."""
        
        self._run_make(directory='.', target='mochitest-chrome')

    def run_browser_chrome_suite(self):
        """Runs browser chrome mochitests."""
        
        self._run_make(directory='.', target='mochitest-browser-chrome')

    def run_all(self):
        self.run_plain_suite()
        self.run_chrome_suite()
        self.run_browser_chrome_suite()

    def run_mochitest_test(self, test_file=None, suite=None):
        """Runs a mochitest.

        test_file is a path to a test file. It can be a relative path from the
        top source directory, an absolute filename, or a directory containing
        test files.

        suite is the type of mochitest to run. It can be one of ('plain',
        'chrome', 'browser').
        """

        
        target = None
        if suite == 'plain':
            target = 'mochitest-plain'
        elif suite == 'chrome':
            target = 'mochitest-chrome'
        elif suite == 'browser':
            target = 'mochitest-browser-chrome'
        else:
            raise Exception('None or unrecognized mochitest suite type.')

        if test_file:
            path = self._parse_test_path(test_file)['normalized']
            if not os.path.exists(path):
                raise Exception('No manifest file was found at %s.' % path)
            env = {'TEST_PATH': path}
        else:
            env = {}

        self._run_make(directory='.', target=target, append_env=env)

    def _parse_test_path(self, test_path):
        is_dir = os.path.isdir(test_path)

        if is_dir and not test_path.endswith(os.path.sep):
            test_path += os.path.sep

        normalized = test_path

        if test_path.startswith(self.topsrcdir):
            normalized = test_path[len(self.topsrcdir):]

        return {
            'normalized': normalized,
            'is_dir': is_dir,
        }
