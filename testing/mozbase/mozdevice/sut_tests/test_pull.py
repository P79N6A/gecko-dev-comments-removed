



import hashlib
import os
import posixpath

from dmunit import DeviceManagerTestCase
from mozdevice.devicemanager import DMError

class PullTestCase(DeviceManagerTestCase):

    def runTest(self):
        """Tests the "pull" command with a binary file.
        """
        orig = hashlib.md5()
        new = hashlib.md5()
        local_test_file = os.path.join('test-files', 'mybinary.zip')
        orig.update(file(local_test_file, 'r').read())

        testroot = self.dm.getDeviceRoot()
        remote_test_file = posixpath.join(testroot, 'mybinary.zip')
        self.dm.removeFile(remote_test_file)
        self.dm.pushFile(local_test_file, remote_test_file)
        new.update(self.dm.pullFile(remote_test_file))
        
        
        self.assertEqual(orig.hexdigest(), new.hexdigest())

        remote_missing_file = posixpath.join(testroot, 'doesnotexist')
        self.dm.removeFile(remote_missing_file)  
        self.assertRaises(DMError, self.dm.pullFile, remote_missing_file)
