

import mozfile
import mozinfo
import os
import shutil
import tempfile
import unittest
import stubs


class TestRemoveTree(unittest.TestCase):
    """test our ability to remove a directory tree"""

    def setUp(self):
        
        self.tempdir = stubs.create_stub()

    def tearDown(self):
        
        if os.path.isdir(self.tempdir):
            mozfile.rmtree(self.tempdir)

    def test_remove_directory(self):
        self.assertTrue(os.path.isdir(self.tempdir))
        try:
            mozfile.rmtree(self.tempdir)
        except:
            shutil.rmtree(self.tempdir)
            raise
        self.assertFalse(os.path.exists(self.tempdir))

    def test_remove_directory_with_open_file(self):
        """ Tests handling when removing a directory tree
            which has a file in it is still open """
        
        filepath = os.path.join(self.tempdir, *stubs.files[1])
        f = file(filepath, 'w')
        f.write('foo-bar')
        
        if mozinfo.isWin:
            
            self.assertRaises(WindowsError, mozfile.rmtree, self.tempdir)
        else:
            
            mozfile.rmtree(self.tempdir)
            self.assertFalse(os.path.exists(self.tempdir))

    def test_remove_directory_after_closing_file(self):
        """ Test that the call to mozfile.rmtree succeeds on
            all platforms after file is closed """

        filepath = os.path.join(self.tempdir, *stubs.files[1])
        with open(filepath, 'w') as f:
            f.write('foo-bar')
        
        mozfile.rmtree(self.tempdir)
        
        self.assertFalse(os.path.exists(self.tempdir))
