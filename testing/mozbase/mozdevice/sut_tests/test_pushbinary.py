



import os
import posixpath

from dmunit import DeviceManagerTestCase

class PushBinaryTestCase(DeviceManagerTestCase):

    def runTest(self):
        """This tests copying a binary file.
        """
        testroot = self.dm.getDeviceRoot()
        self.dm.removeFile(posixpath.join(testroot, 'mybinary.zip'))
        self.dm.pushFile(os.path.join('test-files', 'mybinary.zip'),
                         posixpath.join(testroot, 'mybinary.zip'))
