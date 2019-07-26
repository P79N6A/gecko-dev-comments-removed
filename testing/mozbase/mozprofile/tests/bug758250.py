

import mozprofile
import os
import shutil
import tempfile
import unittest


here = os.path.dirname(os.path.abspath(__file__))


class Bug758250(unittest.TestCase):
    """
    use of --profile in mozrunner just blows away addon sources:
    https://bugzilla.mozilla.org/show_bug.cgi?id=758250
    """

    def setUp(self):
        self.tmpdir = tempfile.mkdtemp()
        self.addon = os.path.join(here, 'addons', 'empty')

    def tearDown(self):
        
        shutil.rmtree(self.tmpdir)

    def test_profile_addon_cleanup(self):

        
        self.assertTrue(os.path.exists(self.addon))
        self.assertTrue(os.path.isdir(self.addon))
        self.assertTrue(os.path.exists(os.path.join(self.addon, 'install.rdf')))

        
        shutil.rmtree(self.tmpdir)
        shutil.copytree(self.addon, self.tmpdir)
        self.assertTrue(os.path.exists(os.path.join(self.tmpdir, 'install.rdf')))

        
        profile = mozprofile.FirefoxProfile()
        path = profile.profile

        
        newprofile = mozprofile.FirefoxProfile(profile=path, addons=[self.tmpdir])
        newprofile.cleanup()

        
        self.assertTrue(os.path.exists(self.tmpdir))
        self.assertTrue(os.path.exists(os.path.join(self.tmpdir, 'install.rdf')))


if __name__ == '__main__':
    unittest.main()
