



import os
import posixpath
from StringIO import StringIO

from dmunit import DeviceManagerTestCase

class ExecEnvTestCase(DeviceManagerTestCase):

    def runTest(self):
        """Exec test with env vars."""
        
        localfile = os.path.join('test-files', 'test_script.sh')
        remotefile = posixpath.join(self.dm.getDeviceRoot(), 'test_script.sh')
        self.dm.pushFile(localfile, remotefile)

        
        out = StringIO()
        self.dm.shell(['sh', remotefile], out, env={'THE_ANSWER': 42})

        
        out.seek(0)
        
        line = out.readline()
        self.assertTrue(int(line) == 42)

        
        self.dm.removeFile(remotefile)
