

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

    def test_profile_addon_cleanup(self):

        
        empty = os.path.join(here, 'addons', 'empty')
        self.assertTrue(os.path.exists(empty))
        self.assertTrue(os.path.isdir(empty))
        self.assertTrue(os.path.exists(os.path.join(empty, 'install.rdf')))

        
        tmpdir = tempfile.mktemp()
        shutil.copytree(empty, tmpdir)
        self.assertTrue(os.path.exists(os.path.join(tmpdir, 'install.rdf')))

        
        profile = mozprofile.FirefoxProfile()
        path = profile.profile

        
        newprofile = mozprofile.FirefoxProfile(profile=path, addons=[tmpdir])
        newprofile.cleanup()

        
        self.assertTrue(os.path.exists(tmpdir))
        self.assertTrue(os.path.exists(os.path.join(tmpdir, 'install.rdf')))

        
        shutil.rmtree(tmpdir)

if __name__ == '__main__':
    unittest.main()
