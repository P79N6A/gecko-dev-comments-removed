



import tempfile
import posixpath

from dmunit import DeviceManagerTestCase

class FileExistsTestCase(DeviceManagerTestCase):
    """This tests the "fileExists" command.
    """

    def testOnRoot(self):
        self.assertTrue(self.dm.fileExists('/'))

    def testOnNonexistent(self):
        self.assertFalse(self.dm.fileExists('/doesNotExist'))

    def testOnRegularFile(self):
        remote_path = posixpath.join(self.dm.deviceRoot, 'testFile')
        self.assertFalse(self.dm.fileExists(remote_path))
        with tempfile.NamedTemporaryFile() as f:
            self.dm.pushFile(f.name, remote_path)
        self.assertTrue(self.dm.fileExists(remote_path))
        self.dm.removeFile(remote_path)

    def testOnDirectory(self):
        remote_path = posixpath.join(self.dm.deviceRoot, 'testDir')
        remote_path_file = posixpath.join(remote_path, 'testFile')
        self.assertFalse(self.dm.fileExists(remote_path))
        with tempfile.NamedTemporaryFile() as f:
            self.dm.pushFile(f.name, remote_path_file)
        self.assertTrue(self.dm.fileExists(remote_path))
        self.dm.removeFile(remote_path_file)
        self.dm.removeDir(remote_path)

