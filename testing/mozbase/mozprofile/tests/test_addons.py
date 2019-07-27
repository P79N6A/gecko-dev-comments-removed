





import os
import shutil
import tempfile
import unittest
import urllib2

from manifestparser import ManifestParser
import mozfile
import mozhttpd
import mozlog.unstructured as mozlog
import mozprofile

from addon_stubs import generate_addon, generate_manifest


here = os.path.dirname(os.path.abspath(__file__))


class TestAddonsManager(unittest.TestCase):
    """ Class to test mozprofile.addons.AddonManager """

    def setUp(self):
        self.logger = mozlog.getLogger('mozprofile.addons')
        self.logger.setLevel(mozlog.ERROR)

        self.profile = mozprofile.profile.Profile()
        self.am = self.profile.addon_manager

        self.profile_path = self.profile.profile
        self.tmpdir = tempfile.mkdtemp()
        self.addCleanup(mozfile.remove, self.tmpdir)

    def test_install_addons_multiple_same_source(self):
        
        addon_xpi = generate_addon('test-addon-1@mozilla.org',
                                   path=self.tmpdir)
        addon_folder = generate_addon('test-addon-1@mozilla.org',
                                      path=self.tmpdir,
                                      xpi=False)

        
        self.am.install_addons([addon_folder, addon_folder])
        self.assertEqual(self.am.installed_addons, [addon_folder])
        self.am.clean()

        
        self.am.install_addons([addon_xpi, addon_xpi])
        self.assertEqual(self.am.installed_addons, [addon_xpi])
        self.am.clean()

        
        
        self.am.install_addons([addon_folder, addon_xpi])
        self.assertEqual(len(self.am.installed_addons), 2)
        self.assertIn(addon_folder, self.am.installed_addons)
        self.assertIn(addon_xpi, self.am.installed_addons)
        self.am.clean()

    def test_download(self):
        server = mozhttpd.MozHttpd(docroot=os.path.join(here, 'addons'))
        server.start()

        
        
        try:
            addon = server.get_url() + 'empty.xpi'
            xpi_file = mozprofile.addons.AddonManager.download(addon)
            self.assertTrue(os.path.isfile(xpi_file))
            self.assertIn('test-empty@quality.mozilla.org.xpi',
                          os.path.basename(xpi_file))
            self.assertNotIn(self.tmpdir, os.path.dirname(xpi_file))
        finally:
            
            
            if os.path.isfile(xpi_file):
                os.remove(xpi_file)

        
        addon = server.get_url() + 'empty.xpi'
        xpi_file = self.am.download(addon, self.tmpdir)
        self.assertTrue(os.path.isfile(xpi_file))
        self.assertIn('test-empty@quality.mozilla.org.xpi',
                      os.path.basename(xpi_file))
        self.assertIn(self.tmpdir, os.path.dirname(xpi_file))
        self.assertEqual(self.am.downloaded_addons, [])
        os.remove(xpi_file)

        
        addon = server.get_url() + 'invalid.xpi'
        self.assertRaises(mozprofile.addons.AddonFormatError,
                          self.am.download, addon, self.tmpdir)
        self.assertEqual(os.listdir(self.tmpdir), [])

        
        addon = server.get_url() + 'not_existent.xpi'
        self.assertRaises(urllib2.HTTPError,
                          self.am.download, addon, self.tmpdir)
        self.assertEqual(os.listdir(self.tmpdir), [])

        
        addon = 'not_existent.xpi'
        self.assertRaises(ValueError,
                          self.am.download, addon, self.tmpdir)
        self.assertEqual(os.listdir(self.tmpdir), [])

        server.stop()

    def test_install_from_path_xpi(self):
        addons_to_install = []
        addons_installed = []

        
        for ext in ['test-addon-1@mozilla.org', 'test-addon-2@mozilla.org']:
            temp_addon = generate_addon(ext, path=self.tmpdir)
            addons_to_install.append(self.am.addon_details(temp_addon)['id'])
            self.am.install_from_path(temp_addon)

        
        addons_installed = [unicode(x[:-len('.xpi')]) for x in os.listdir(os.path.join(
                            self.profile.profile, 'extensions', 'staged'))]
        self.assertEqual(addons_to_install.sort(), addons_installed.sort())

    def test_install_from_path_folder(self):
        
        addons = []
        addons.append(generate_addon('test-addon-1@mozilla.org',
                                     path=self.tmpdir))
        addons.append(generate_addon('test-addon-2@mozilla.org',
                                     path=self.tmpdir,
                                     xpi=False))
        addons.append(generate_addon('test-addon-3@mozilla.org',
                                     path=self.tmpdir,
                                     name='addon-3'))
        addons.append(generate_addon('test-addon-4@mozilla.org',
                                     path=self.tmpdir,
                                     name='addon-4',
                                     xpi=False))
        addons.sort()

        self.am.install_from_path(self.tmpdir)

        self.assertEqual(self.am.installed_addons, addons)

    def test_install_from_path_unpack(self):
        
        addon_xpi = generate_addon('test-addon-unpack@mozilla.org',
                                   path=self.tmpdir)
        addon_folder = generate_addon('test-addon-unpack@mozilla.org',
                                      path=self.tmpdir,
                                      xpi=False)
        addon_no_unpack = generate_addon('test-addon-1@mozilla.org',
                                         path=self.tmpdir)

        
        self.am.install_from_path(addon_xpi)
        self.assertEqual(self.am.installed_addons, [addon_xpi])
        self.am.clean()

        
        self.am.install_from_path(addon_folder)
        self.assertEqual(self.am.installed_addons, [addon_folder])
        self.am.clean()

        
        self.am.install_from_path(addon_no_unpack, unpack=True)
        self.assertEqual(self.am.installed_addons, [addon_no_unpack])
        self.am.clean()

    def test_install_from_path_url(self):
        server = mozhttpd.MozHttpd(docroot=os.path.join(here, 'addons'))
        server.start()

        addon = server.get_url() + 'empty.xpi'
        self.am.install_from_path(addon)

        server.stop()

        self.assertEqual(len(self.am.downloaded_addons), 1)
        self.assertTrue(os.path.isfile(self.am.downloaded_addons[0]))
        self.assertIn('test-empty@quality.mozilla.org.xpi',
                      os.path.basename(self.am.downloaded_addons[0]))

    def test_install_from_path_after_reset(self):
        
        addon = generate_addon('test-addon-1@mozilla.org',
                               path=self.tmpdir, xpi=False)

        
        self.profile.addon_manager.install_from_path(addon)

        self.profile.reset()

        self.profile.addon_manager.install_from_path(addon)
        self.assertEqual(self.profile.addon_manager.installed_addons, [addon])

    def test_install_from_path_backup(self):
        staged_path = os.path.join(self.profile_path, 'extensions', 'staged')

        
        addon_xpi = generate_addon('test-addon-1@mozilla.org',
                                   path=self.tmpdir)
        addon_folder = generate_addon('test-addon-1@mozilla.org',
                                      path=self.tmpdir,
                                      xpi=False)
        addon_name = generate_addon('test-addon-1@mozilla.org',
                                    path=self.tmpdir,
                                    name='test-addon-1-dupe@mozilla.org')

        
        self.am.install_from_path(addon_xpi)
        self.assertIsNone(self.am.backup_dir)

        self.am.install_from_path(addon_xpi)
        self.assertIsNotNone(self.am.backup_dir)
        self.assertEqual(os.listdir(self.am.backup_dir),
                         ['test-addon-1@mozilla.org.xpi'])

        self.am.clean()
        self.assertEqual(os.listdir(staged_path),
                         ['test-addon-1@mozilla.org.xpi'])
        self.am.clean()

        
        self.am.install_from_path(addon_folder)
        self.assertIsNone(self.am.backup_dir)

        self.am.install_from_path(addon_folder)
        self.assertIsNotNone(self.am.backup_dir)
        self.assertEqual(os.listdir(self.am.backup_dir),
                         ['test-addon-1@mozilla.org'])

        self.am.clean()
        self.assertEqual(os.listdir(staged_path),
                         ['test-addon-1@mozilla.org'])
        self.am.clean()

        
        self.am.install_from_path(addon_name)
        self.assertIsNone(self.am.backup_dir)

        self.am.install_from_path(addon_xpi)
        self.assertIsNotNone(self.am.backup_dir)
        self.assertEqual(os.listdir(self.am.backup_dir),
                         ['test-addon-1@mozilla.org.xpi'])

        self.am.clean()
        self.assertEqual(os.listdir(staged_path),
                         ['test-addon-1@mozilla.org.xpi'])
        self.am.clean()

    def test_install_from_path_invalid_addons(self):
        
        addons = []
        addons.append(generate_addon('test-addon-invalid-no-manifest@mozilla.org',
                      path=self.tmpdir,
                      xpi=False))
        addons.append(generate_addon('test-addon-invalid-no-id@mozilla.org',
                      path=self.tmpdir))

        self.am.install_from_path(self.tmpdir)

        self.assertEqual(self.am.installed_addons, [])

    @unittest.skip("Feature not implemented as part of AddonManger")
    def test_install_from_path_error(self):
        """ Check install_from_path raises an error with an invalid addon"""

        temp_addon = generate_addon('test-addon-invalid-version@mozilla.org')
        
        self.am.install_from_path(temp_addon)

    def test_install_from_manifest(self):
        temp_manifest = generate_manifest(['test-addon-1@mozilla.org',
                                           'test-addon-2@mozilla.org'])
        m = ManifestParser()
        m.read(temp_manifest)
        addons = m.get()

        
        addons_to_install = [self.am.addon_details(x['path']).get('id') for x in addons]

        self.am.install_from_manifest(temp_manifest)
        
        addons_installed = [unicode(x[:-len('.xpi')]) for x in os.listdir(os.path.join(
                            self.profile.profile, 'extensions', 'staged'))]
        self.assertEqual(addons_installed.sort(), addons_to_install.sort())

        
        mozfile.rmtree(os.path.dirname(temp_manifest))

    def test_addon_details(self):
        
        valid_addon = generate_addon('test-addon-1@mozilla.org',
                                     path=self.tmpdir)
        invalid_addon = generate_addon('test-addon-invalid-not-wellformed@mozilla.org',
                                       path=self.tmpdir)

        
        details = self.am.addon_details(valid_addon)
        self.assertEqual(details['id'], 'test-addon-1@mozilla.org')
        self.assertEqual(details['name'], 'Test Add-on 1')
        self.assertEqual(details['unpack'], False)
        self.assertEqual(details['version'], '0.1')

        
        self.assertRaises(mozprofile.addons.AddonFormatError,
                          self.am.addon_details, invalid_addon)

        
        self.assertRaises(IOError,
                          self.am.addon_details, '')

        
        addon_path = os.path.join(os.path.join(here, 'files'), 'not_an_addon.txt')
        self.assertRaises(mozprofile.addons.AddonFormatError,
                          self.am.addon_details, addon_path)

    @unittest.skip("Bug 900154")
    def test_clean_addons(self):
        addon_one = generate_addon('test-addon-1@mozilla.org')
        addon_two = generate_addon('test-addon-2@mozilla.org')

        self.am.install_addons(addon_one)
        installed_addons = [unicode(x[:-len('.xpi')]) for x in os.listdir(os.path.join(
                            self.profile.profile, 'extensions', 'staged'))]

        
        
        
        duplicate_profile = mozprofile.profile.Profile(profile=self.profile.profile,
                                                       addons=addon_two)
        duplicate_profile.addon_manager.clean()

        addons_after_cleanup = [unicode(x[:-len('.xpi')]) for x in os.listdir(os.path.join(
                                duplicate_profile.profile, 'extensions', 'staged'))]
        
        self.assertEqual(installed_addons, addons_after_cleanup)

    def test_noclean(self):
        """test `restore=True/False` functionality"""

        server = mozhttpd.MozHttpd(docroot=os.path.join(here, 'addons'))
        server.start()

        profile = tempfile.mkdtemp()
        tmpdir = tempfile.mkdtemp()

        try:
            
            self.assertFalse(bool(os.listdir(profile)))

            
            addons = []
            addons.append(generate_addon('test-addon-1@mozilla.org',
                                         path=tmpdir))
            addons.append(server.get_url() + 'empty.xpi')

            
            am = mozprofile.addons.AddonManager(profile, restore=True)

            for addon in addons:
                am.install_from_path(addon)

            
            self.assertEqual(os.listdir(profile), ['extensions'])
            staging_folder = os.path.join(profile, 'extensions', 'staged')
            self.assertTrue(os.path.exists(staging_folder))
            self.assertEqual(len(os.listdir(staging_folder)), 2)

            
            downloaded_addons = am.downloaded_addons
            del am

            self.assertEqual(os.listdir(profile), ['extensions'])
            self.assertTrue(os.path.exists(staging_folder))
            self.assertEqual(os.listdir(staging_folder), [])

            for addon in downloaded_addons:
                self.assertFalse(os.path.isfile(addon))

        finally:
            mozfile.rmtree(tmpdir)
            mozfile.rmtree(profile)

    def test_remove_addon(self):
        addons = []
        addons.append(generate_addon('test-addon-1@mozilla.org',
                                     path=self.tmpdir))
        addons.append(generate_addon('test-addon-2@mozilla.org',
                                     path=self.tmpdir))

        self.am.install_from_path(self.tmpdir)

        extensions_path = os.path.join(self.profile_path, 'extensions')
        staging_path = os.path.join(extensions_path, 'staged')

        
        shutil.move(os.path.join(staging_path, 'test-addon-1@mozilla.org.xpi'),
                    extensions_path)

        for addon in self.am._addons:
            self.am.remove_addon(addon)

        self.assertEqual(os.listdir(staging_path), [])
        self.assertEqual(os.listdir(extensions_path), ['staged'])


if __name__ == '__main__':
    unittest.main()
