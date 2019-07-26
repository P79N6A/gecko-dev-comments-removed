



import os
import posixpath

from dmunit import DeviceManagerTestCase

class PushSmallTextTestCase(DeviceManagerTestCase):

    def runTest(self):
        """This tests copying a small text file.
        """
        testroot = self.dm.getDeviceRoot()
        self.dm.removeFile(posixpath.join(testroot, 'smalltext.txt'))
        self.dm.pushFile(os.path.join('test-files', 'smalltext.txt'),
                         posixpath.join(testroot, 'smalltext.txt'))
