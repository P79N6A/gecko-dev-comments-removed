



from __future__ import unicode_literals

import os
import sys
import unittest

from mozfile.mozfile import NamedTemporaryFile

from mozunit import main

from mach.logging import LoggingManager

from mozbuild.base import (
    BuildConfig,
    MozbuildObject,
)

from mozbuild.backend.configenvironment import ConfigEnvironment



curdir = os.path.dirname(__file__)
topsrcdir = os.path.normpath(os.path.join(curdir, '..', '..', '..', '..'))
log_manager = LoggingManager()


class TestMozbuildObject(unittest.TestCase):
    def get_base(self):
        return MozbuildObject(topsrcdir, None, log_manager)

    def test_objdir_config_guess(self):
        base = self.get_base()

        with NamedTemporaryFile() as mozconfig:
            os.environ[b'MOZCONFIG'] = mozconfig.name

            self.assertIsNotNone(base.topobjdir)
            self.assertEqual(len(base.topobjdir.split()), 1)
            self.assertTrue(base.topobjdir.endswith(base._config_guess))

        del os.environ[b'MOZCONFIG']

    def test_config_guess(self):
        
        
        base = self.get_base()
        result = base._config_guess

        self.assertIsNotNone(result)
        self.assertGreater(len(result), 0)

    def test_config_environment(self):
        base = self.get_base()

        
        
        
        ce = base.config_environment
        self.assertIsInstance(ce, ConfigEnvironment)

        self.assertEqual(base.defines, ce.defines)
        self.assertEqual(base.substs, ce.substs)

        self.assertIsInstance(base.defines, dict)
        self.assertIsInstance(base.substs, dict)

    def test_get_binary_path(self):
        base = self.get_base()

        p = base.get_binary_path('xpcshell', False)
        platform = sys.platform
        if platform.startswith('darwin'):
            self.assertTrue(p.endswith('Contents/MacOS/xpcshell'))
        elif platform.startswith('win32', 'cygwin'):
            self.assertTrue(p.endswith('xpcshell.exe'))
        else:
            self.assertTrue(p.endswith('dist/bin/xpcshell'))


if __name__ == '__main__':
    main()
