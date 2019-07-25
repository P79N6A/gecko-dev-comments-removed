



import os
import posixpath

from dmunit import DeviceManagerTestCase


class UnzipTestCase(DeviceManagerTestCase):

    def runTest(self):
        """ This tests unzipping a file on the device.
        """
        testroot = posixpath.join(self.dm.getDeviceRoot(), 'infratest')
        self.dm.removeDir(testroot)
        self.dm.mkDir(testroot)
        self.assert_(self.dm.pushFile(
            os.path.join('test-files', 'mybinary.zip'),
            posixpath.join(testroot, 'mybinary.zip')))
        self.assertNotEqual(self.dm.unpackFile(
            posixpath.join(testroot, 'mybinary.zip')), None)
        
        
        self.assert_(self.dm.dirExists(
            posixpath.join(testroot, 'push2', 'sub1')))
        self.assert_(self.dm.validateFile(
            posixpath.join(testroot, 'push2', 'sub1', 'file1.txt'),
            os.path.join('test-files', 'push2', 'sub1', 'file1.txt')))
        self.assert_(self.dm.validateFile(
            posixpath.join(testroot, 'push2', 'sub1', 'sub1.1', 'file2.txt'),
            os.path.join('test-files', 'push2', 'sub1', 'sub1.1', 'file2.txt')))
        self.assert_(self.dm.validateFile(
            posixpath.join(testroot, 'push2', 'sub2', 'file3.txt'),
            os.path.join('test-files', 'push2', 'sub2', 'file3.txt')))
        self.assert_(self.dm.validateFile(
            posixpath.join(testroot, 'push2', 'file4.bin'),
            os.path.join('test-files', 'push2', 'file4.bin')))

        
        newdir = posixpath.join(testroot, 'newDir')
        self.dm.mkDir(newdir)

        self.assertNotEqual(self.dm.unpackFile(
            posixpath.join(testroot, 'mybinary.zip'), newdir), None)

        self.assert_(self.dm.dirExists(posixpath.join(newdir, 'push2', 'sub1')))
        self.assert_(self.dm.validateFile(
            posixpath.join(newdir, 'push2', 'sub1', 'file1.txt'),
            os.path.join('test-files', 'push2', 'sub1', 'file1.txt')))
        self.assert_(self.dm.validateFile(
            posixpath.join(newdir, 'push2', 'sub1', 'sub1.1', 'file2.txt'),
            os.path.join('test-files', 'push2', 'sub1', 'sub1.1', 'file2.txt')))
        self.assert_(self.dm.validateFile(
            posixpath.join(newdir, 'push2', 'sub2', 'file3.txt'),
            os.path.join('test-files', 'push2', 'sub2', 'file3.txt')))
        self.assert_(self.dm.validateFile(
            posixpath.join(newdir, 'push2', 'file4.bin'),
            os.path.join('test-files', 'push2', 'file4.bin')))
