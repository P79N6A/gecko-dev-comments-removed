



import os
import posixpath

from dmunit import DeviceManagerTestCase


class IsDirTestCase(DeviceManagerTestCase):

    def runTest(self):
        """This tests the isDir() function.
        """
        testroot = posixpath.join(self.dm.getDeviceRoot(), 'infratest')
        self.dm.removeDir(testroot)
        self.dm.mkDir(testroot)
        self.assertTrue(self.dm.isDir(testroot))
        testdir = posixpath.join(testroot, 'testdir')
        self.assertFalse(self.dm.isDir(testdir))
        self.dm.mkDir(testdir)
        self.assertTrue(self.dm.isDir(testdir))
        self.dm.pushFile(os.path.join('test-files', 'mytext.txt'),
                         posixpath.join(testdir, 'mytext.txt'))
        self.assertFalse(self.dm.isDir(posixpath.join(testdir, 'mytext.txt')))
        self.dm.removeDir(testroot)
        self.assertFalse(self.dm.isDir(testroot))
        self.assertFalse(self.dm.isDir(testdir))
        self.assertFalse(self.dm.isDir(posixpath.join(testdir, 'mytext.txt')))
        self.assertFalse(self.dm.isDir(posixpath.join('/', 'noroot', 'nosub')))
