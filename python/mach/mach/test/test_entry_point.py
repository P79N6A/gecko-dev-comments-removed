


from __future__ import unicode_literals

import imp
import os
import sys

from mach.base import MachError
from mach.test.common import TestBase
from mock import patch

here = os.path.abspath(os.path.dirname(__file__))

class Entry():
    """Stub replacement for pkg_resources.EntryPoint"""
    def __init__(self, providers):
        self.providers = providers

    def load(self):
        def _providers():
            return self.providers
        return _providers

class TestEntryPoints(TestBase):
    """Test integrating with setuptools entry points"""
    provider_dir = os.path.join(here, 'providers')

    def _run_mach(self):
        return TestBase._run_mach(self, ['help'], entry_point='mach.providers')

    @patch('pkg_resources.iter_entry_points')
    def test_load_entry_point_from_directory(self, mock):
        
        
        if b'mach.commands' not in sys.modules:
            mod = imp.new_module(b'mach.commands')
            sys.modules[b'mach.commands'] = mod

        mock.return_value = [Entry(['providers'])]
        
        with self.assertRaises(MachError):
            self._run_mach()

    @patch('pkg_resources.iter_entry_points')
    def test_load_entry_point_from_file(self, mock):
        mock.return_value = [Entry([os.path.join('providers', 'basic.py')])]

        result, stdout, stderr = self._run_mach()
        self.assertIsNone(result)
        self.assertIn('cmd_foo', stdout)
