



import posixpath
from StringIO import StringIO

from dmunit import DeviceManagerTestCase

class ExecTestCase(DeviceManagerTestCase):

    def runTest(self):
        """Simple exec test, does not use env vars."""
        out = StringIO()
        filename = posixpath.join(self.dm.getDeviceRoot(), 'test_exec_file')
        
        self.dm.removeFile(filename)
        self.dm.shell(['dd', 'if=/dev/zero', 'of=%s' % filename, 'bs=1024',
                       'count=1'], out)
        
        self.assertTrue(self.dm.fileExists(filename))
        
        self.dm.removeFile(filename)
