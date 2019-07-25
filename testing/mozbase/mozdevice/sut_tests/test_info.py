



from dmunit import DeviceManagerTestCase


class InfoTestCase(DeviceManagerTestCase):

    runs_on_test_device = False

    def runTest(self):
        """ This tests the "info" command
        """
        cmds = ('os', 'id', 'systime', 'uptime', 'screen',
                'memory', 'power')
        for c in cmds:
            data = self.dm.getInfo(c)
            print c + str(data)

        print " ==== Now we call them all ===="
        
        

        
