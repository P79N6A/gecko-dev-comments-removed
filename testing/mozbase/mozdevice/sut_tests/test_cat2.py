



import hashlib
import os
import posixpath

from dmunit import DeviceManagerTestCase


class Cat2TestCase(DeviceManagerTestCase):

    def runTest(self):
        """This tests copying a binary file to and from the device the binary.
           File is > 64K.
        """
        testroot = posixpath.join(self.dm.getDeviceRoot(), 'infratest')
        self.dm.removeDir(testroot)
        self.dm.mkDir(testroot)
        origFile = open(os.path.join('test-files', 'mybinary.zip'), 'rb').read()
        self.dm.pushFile(
                         os.path.join('test-files', 'mybinary.zip'),
                         posixpath.join(testroot, 'mybinary.zip'))
        resultFile = self.dm.catFile(posixpath.join(testroot, 'mybinary.zip'))
        self.assertEqual(hashlib.md5(origFile).hexdigest(),
                         hashlib.md5(resultFile).hexdigest())
