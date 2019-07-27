





import mozinfo
import mozinstall
import mozfile
import os
import tempfile
import unittest


here = os.path.dirname(os.path.abspath(__file__))

class TestMozInstall(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        """ Setting up stub installers """
        cls.dmg = os.path.join(here, 'Installer-Stubs', 'firefox.dmg')
        
        
        cls.exe = os.path.join(here, 'Installer-Stubs', 'firefox.exe')
        cls.zipfile = os.path.join(here, 'Installer-Stubs', 'firefox.zip')
        cls.bz2 = os.path.join(here, 'Installer-Stubs', 'firefox.tar.bz2')

    def setUp(self):
        self.tempdir = tempfile.mkdtemp()

    def tearDown(self):
        mozfile.rmtree(self.tempdir)

    @unittest.skipIf(mozinfo.isWin, "Bug 1157352 - We need a new firefox.exe for mozinstall 1.12 and higher.")
    def test_get_binary(self):
        """ Test mozinstall's get_binary method """

        if mozinfo.isLinux:
            installdir = mozinstall.install(self.bz2, self.tempdir)
            binary = os.path.join(installdir, 'firefox')
            self.assertEqual(binary, mozinstall.get_binary(installdir, 'firefox'))

        elif mozinfo.isWin:
            installdir_exe = mozinstall.install(self.exe,
                                                os.path.join(self.tempdir, 'exe'))
            binary_exe = os.path.join(installdir_exe, 'core', 'firefox.exe')
            self.assertEqual(binary_exe, mozinstall.get_binary(installdir_exe,
                             'firefox'))

            installdir_zip = mozinstall.install(self.zipfile,
                                                os.path.join(self.tempdir, 'zip'))
            binary_zip = os.path.join(installdir_zip, 'firefox.exe')
            self.assertEqual(binary_zip, mozinstall.get_binary(installdir_zip,
                             'firefox'))

        elif mozinfo.isMac:
            installdir = mozinstall.install(self.dmg, self.tempdir)
            binary = os.path.join(installdir, 'Contents', 'MacOS', 'firefox')
            self.assertEqual(binary, mozinstall.get_binary(installdir, 'firefox'))

    def test_get_binary_error(self):
        """ Test an InvalidBinary error is raised """

        tempdir_empty = tempfile.mkdtemp()
        self.assertRaises(mozinstall.InvalidBinary, mozinstall.get_binary,
                          tempdir_empty, 'firefox')
        mozfile.rmtree(tempdir_empty)

    @unittest.skipIf(mozinfo.isWin, "Bug 1157352 - We need a new firefox.exe for mozinstall 1.12 and higher.")
    def test_is_installer(self):
        """ Test we can identify a correct installer """

        if mozinfo.isLinux:
            self.assertTrue(mozinstall.is_installer(self.bz2))

        if mozinfo.isWin:
            
            self.assertTrue(mozinstall.is_installer(self.zipfile))

            
            self.assertTrue(mozinstall.is_installer(self.exe))

            try:
                
                
                import pefile
                stub_exe = os.path.join(here, 'build_stub', 'firefox.exe')
                self.assertFalse(mozinstall.is_installer(stub_exe))
            except ImportError:
                pass

        if mozinfo.isMac:
            self.assertTrue(mozinstall.is_installer(self.dmg))

    def test_invalid_source_error(self):
        """ Test InvalidSource error is raised with an incorrect installer """

        if mozinfo.isLinux:
            self.assertRaises(mozinstall.InvalidSource, mozinstall.install,
                              self.dmg, 'firefox')

        elif mozinfo.isWin:
            self.assertRaises(mozinstall.InvalidSource, mozinstall.install,
                              self.bz2, 'firefox')

        elif mozinfo.isMac:
            self.assertRaises(mozinstall.InvalidSource, mozinstall.install,
                              self.bz2, 'firefox')

    @unittest.skipIf(mozinfo.isWin, "Bug 1157352 - We need a new firefox.exe for mozinstall 1.12 and higher.")
    def test_install(self):
        """ Test mozinstall's install capability """

        if mozinfo.isLinux:
            installdir = mozinstall.install(self.bz2, self.tempdir)
            self.assertEqual(os.path.join(self.tempdir, 'firefox'), installdir)

        elif mozinfo.isWin:
            installdir_exe = mozinstall.install(self.exe,
                                                os.path.join(self.tempdir, 'exe'))
            self.assertEqual(os.path.join(self.tempdir, 'exe', 'firefox'),
                             installdir_exe)

            installdir_zip = mozinstall.install(self.zipfile,
                                                os.path.join(self.tempdir, 'zip'))
            self.assertEqual(os.path.join(self.tempdir, 'zip', 'firefox'),
                             installdir_zip)

        elif mozinfo.isMac:
            installdir = mozinstall.install(self.dmg, self.tempdir)
            self.assertEqual(os.path.join(os.path.realpath(self.tempdir),
                                          'FirefoxStub.app'), installdir)

    @unittest.skipIf(mozinfo.isWin, "Bug 1157352 - We need a new firefox.exe for mozinstall 1.12 and higher.")
    def test_uninstall(self):
        """ Test mozinstall's uninstall capabilites """
        

        if mozinfo.isLinux:
            installdir = mozinstall.install(self.bz2, self.tempdir)
            mozinstall.uninstall(installdir)
            self.assertFalse(os.path.exists(installdir))

        elif mozinfo.isWin:
            
            installdir_exe = mozinstall.install(self.exe,
                                                os.path.join(self.tempdir, 'exe'))
            mozinstall.uninstall(installdir_exe)
            self.assertFalse(os.path.exists(installdir_exe))

            
            installdir_zip = mozinstall.install(self.zipfile,
                                                os.path.join(self.tempdir, 'zip'))
            mozinstall.uninstall(installdir_zip)
            self.assertFalse(os.path.exists(installdir_zip))

        elif mozinfo.isMac:
            installdir = mozinstall.install(self.dmg, self.tempdir)
            mozinstall.uninstall(installdir)
            self.assertFalse(os.path.exists(installdir))

if __name__ == '__main__':
    unittest.main()
