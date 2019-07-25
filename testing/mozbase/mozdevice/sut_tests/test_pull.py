



import hashlib
import os
import posixpath

from dmunit import DeviceManagerTestCase


class PullTestCase(DeviceManagerTestCase):

    def runTest(self):
        """Tests the "pull" command with a binary file.
        """
        m_orig = hashlib.md5()
        m_new = hashlib.md5()
        local_test_file = os.path.join('test-files', 'mybinary.zip')
        m_orig.update(file(local_test_file, 'r').read())

        testroot = self.dm.getDeviceRoot()
        remote_test_file = posixpath.join(testroot, 'mybinary.zip')
        self.dm.removeFile(remote_test_file)
        self.dm.pushFile(local_test_file, remote_test_file)
        m_new.update(self.dm.pullFile(remote_test_file))
        
        
        self.assertEqual(m_orig.hexdigest(), m_new.hexdigest())

        remote_missing_file = posixpath.join(testroot, 'doesnotexist')
        self.dm.removeFile(remote_missing_file)  
        self.assertEqual(self.dm.pullFile(remote_missing_file), None)
