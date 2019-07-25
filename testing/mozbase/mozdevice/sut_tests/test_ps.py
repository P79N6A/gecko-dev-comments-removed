



import re

from dmunit import DeviceManagerTestCase


class ProcessListTestCase(DeviceManagerTestCase):

    def runTest(self):
        """ This tests getting a process list from the device
        """
        proclist = self.dm.getProcessList()

        
        
        
        
        procid = re.compile('^[a-f0-9]+')
        procname = re.compile('.+')

        self.assertNotEqual(len(proclist), 0)

        for item in proclist:
            self.assert_(procid.match(item[0]))
            self.assert_(procname.match(item[1]))
