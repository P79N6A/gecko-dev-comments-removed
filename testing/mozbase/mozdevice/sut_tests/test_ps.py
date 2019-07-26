



from dmunit import DeviceManagerTestCase

class ProcessListTestCase(DeviceManagerTestCase):

    def runTest(self):
        """This tests getting a process list from the device.
        """
        proclist = self.dm.getProcessList()

        
        
        
        

        self.assertNotEqual(len(proclist), 0)

        for item in proclist:
            self.assertIsInstance(item[0], int)
            self.assertIsInstance(item[1], str)
            self.assertGreater(len(item[1]), 0)
            if len(item) > 2:
                self.assertIsInstance(item[2], int)

