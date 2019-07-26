



from __future__ import unicode_literals

import imp
import os
import sys
import unittest

from StringIO import StringIO

from mach.main import (
    COMMAND_ERROR,
    MODULE_ERROR
)
from mach.test.common import TestBase


class TestErrorOutput(TestBase):

    def _run_mach(self, args):
        return TestBase._run_mach(self, args, 'throw.py')

    def test_command_error(self):
        result, stdout, stderr = self._run_mach(['throw', '--message',
            'Command Error'])

        self.assertEqual(result, 1)

        self.assertIn(mach.main.COMMAND_ERROR, stdout)

    def test_invoked_error(self):
        result, stdout, stderr = self._run_mach(['throw_deep', '--message',
            'Deep stack'])

        self.assertEqual(result, 1)

        self.assertIn(mach.main.MODULE_ERROR, stdout)
