



import os
import posixpath

from dmunit import DeviceManagerTestCase

class Push2TestCase(DeviceManagerTestCase):

    def runTest(self):
        """This tests copying a directory structure with files to the device.
        """
        testroot = posixpath.join(self.dm.deviceRoot, 'infratest')
        self.dm.removeDir(testroot)
        self.dm.mkDir(testroot)
        path = posixpath.join(testroot, 'push2')
        self.dm.pushDir(os.path.join('test-files', 'push2'), path)

        
        
        
        
        self.assertTrue(
            self.dm.dirExists(posixpath.join(testroot, 'push2', 'sub1')))
        self.assertTrue(self.dm.validateFile(
            posixpath.join(testroot, 'push2', 'sub1', 'file1.txt'),
            os.path.join('test-files', 'push2', 'sub1', 'file1.txt')))
        self.assertTrue(self.dm.validateFile(
            posixpath.join(testroot, 'push2', 'sub1', 'sub1.1', 'file2.txt'),
            os.path.join('test-files', 'push2', 'sub1', 'sub1.1', 'file2.txt')))
        self.assertTrue(self.dm.validateFile(
            posixpath.join(testroot, 'push2', 'sub2', 'file3.txt'),
            os.path.join('test-files', 'push2', 'sub2', 'file3.txt')))
        self.assertTrue(self.dm.validateFile(
            posixpath.join(testroot, 'push2', 'file4.bin'),
            os.path.join('test-files', 'push2', 'file4.bin')))
