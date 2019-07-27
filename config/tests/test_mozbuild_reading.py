



from __future__ import unicode_literals

import os
import unittest

from mozunit import main

from mozbuild.base import MozbuildObject
from mozbuild.frontend.reader import BuildReader


class TestMozbuildReading(unittest.TestCase):
    
    def setUp(self):
        self._old_env = dict(os.environ)
        os.environ.pop('MOZCONFIG', None)
        os.environ.pop('MOZ_OBJDIR', None)

    def tearDown(self):
        os.environ.clear()
        os.environ.update(self._old_env)

    def test_filesystem_traversal_reading(self):
        """Reading moz.build according to filesystem traversal works.

        We attempt to read every known moz.build file via filesystem traversal.

        If this test fails, it means that metadata extraction will fail.
        """
        mb = MozbuildObject.from_environment(detect_virtualenv_mozinfo=False)
        config = mb.config_environment
        reader = BuildReader(config)
        all_paths = set(reader.all_mozbuild_paths())
        paths, contexts = reader.read_relevant_mozbuilds(all_paths)
        self.assertEqual(set(paths), all_paths)
        self.assertGreaterEqual(len(contexts), len(paths))


if __name__ == '__main__':
    main()
