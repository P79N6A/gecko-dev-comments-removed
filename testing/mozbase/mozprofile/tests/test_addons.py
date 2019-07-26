





import addon_stubs
import mozprofile
import mozfile
import tempfile
import os
import unittest
from manifestparser import ManifestParser


class TestAddonsManager(unittest.TestCase):
    """ Class to test mozprofile.addons.AddonManager """

    def setUp(self):
        self.profile = mozprofile.profile.Profile()
        self.am = mozprofile.addons.AddonManager(profile=self.profile.profile)

    def test_install_from_path(self):

        addons_to_install = []
        addons_installed = []

        
        tmpdir = tempfile.mkdtemp()
        for t in ['empty-0-1.xpi', 'another-empty-0-1.xpi']:
            temp_addon = addon_stubs.generate_addon(name=t, path=tmpdir)
            addons_to_install.append(self.am.addon_details(temp_addon)['id'])
            self.am.install_from_path(temp_addon)
        
        addons_installed = [unicode(x[:-len('.xpi')]) for x in os.listdir(os.path.join(
                            self.profile.profile, 'extensions', 'staged'))]
        self.assertEqual(addons_to_install.sort(), addons_installed.sort())
        
        mozfile.rmtree(tmpdir)

    @unittest.skip("Feature not implemented as part of AddonManger")
    def test_install_from_path_error(self):
        """ Check install_from_path raises an error with an invalid addon"""

        temp_addon = addon_stubs.generate_invalid_addon()
        
        self.am.install_from_path(temp_addon)

    def test_install_from_manifest(self):

        temp_manifest = addon_stubs.generate_manifest()
        m = ManifestParser()
        m.read(temp_manifest)
        addons = m.get()
        
        addons_to_install = [self.am.addon_details(x['path'])['id'] for x in addons]

        self.am.install_from_manifest(temp_manifest)
        
        addons_installed = [unicode(x[:-len('.xpi')]) for x in os.listdir(os.path.join(
                            self.profile.profile, 'extensions', 'staged'))]
        self.assertEqual(addons_installed.sort(), addons_to_install.sort())
        
        mozfile.rmtree(os.path.dirname(temp_manifest))

    @unittest.skip("Bug 900154")
    def test_clean_addons(self):

        addon_one = addon_stubs.generate_addon('empty-0-1.xpi')
        addon_two = addon_stubs.generate_addon('another-empty-0-1.xpi')

        self.am.install_addons(addon_one)
        installed_addons = [unicode(x[:-len('.xpi')]) for x in os.listdir(os.path.join(
                            self.profile.profile, 'extensions', 'staged'))]

        
        
        
        duplicate_profile = mozprofile.profile.Profile(profile=self.profile.profile,
                                                       addons=addon_two)
        duplicate_profile.addon_manager.clean_addons()

        addons_after_cleanup = [unicode(x[:-len('.xpi')]) for x in os.listdir(os.path.join(
                                duplicate_profile.profile, 'extensions', 'staged'))]
        
        self.assertEqual(installed_addons, addons_after_cleanup)

    def test_noclean(self):
        """test `restore=True/False` functionality"""

        profile = tempfile.mkdtemp()
        tmpdir = tempfile.mkdtemp()
        try:

            
            self.assertFalse(bool(os.listdir(profile)))

            
            stub = addon_stubs.generate_addon(name='empty-0-1.xpi',
                                              path=tmpdir)

            
            addons  = mozprofile.addons.AddonManager(profile, restore=True)
            addons.install_from_path(stub)

            
            self.assertEqual(os.listdir(profile), ['extensions'])
            extensions = os.path.join(profile, 'extensions', 'staged')
            self.assertTrue(os.path.exists(extensions))
            contents = os.listdir(extensions)
            self.assertEqual(len(contents), 1)

            
            del addons
            self.assertEqual(os.listdir(profile), ['extensions'])
            self.assertTrue(os.path.exists(extensions))
            contents = os.listdir(extensions)
            self.assertEqual(len(contents), 0)

        finally:
            mozfile.rmtree(tmpdir)
            mozfile.rmtree(profile)


if __name__ == '__main__':
    unittest.main()
