



from StringIO import StringIO
import posixpath

from dmunit import DeviceManagerTestCase


class ProcessListTestCase(DeviceManagerTestCase):

    def runTest(self):
        """ simple exec test, does not use env vars """
        out = StringIO()
        filename = posixpath.join(self.dm.getDeviceRoot(), 'test_exec_file')
        
        self.dm.removeFile(filename)
        self.dm.shell(['touch', filename], out)
        
        self.assertTrue(self.dm.fileExists(filename))
        
        self.dm.removeFile(filename)
