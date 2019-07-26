



import socket
from time import strptime
import re

from dmunit import DeviceManagerTestCase


class DataChannelTestCase(DeviceManagerTestCase):

    runs_on_test_device = False

    def runTest(self):
        """ This tests the heartbeat and the data channel
        """
        ip = self.dm.host
        port = 20700

        
        self._datasock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
        self._datasock.settimeout(float(60 * 2))
        self._datasock.connect((ip, port))
        self._connected = True

        
        numbeats = 0
        capturedHeader = False
        while(numbeats < 3):
            data = self._datasock.recv(1024)
            print data
            self.assertNotEqual(len(data), 0)

            
            if not capturedHeader:
                m = re.match(r"(.*?) trace output", data)
                self.assertNotEqual(m, None,
                    'trace output line does not match. The line: ' + str(data))
                lastHeartbeatTime = strptime(m.group(1), "%Y%m%d-%H:%M:%S")
                capturedHeader = True

            
            m = re.match(r"(.*?) Thump thump - (.*)", data)
            if m == None:
                
                
                continue

            
            mHeartbeatTime = m.group(1)
            mHeartbeatTime = strptime(mHeartbeatTime, "%Y%m%d-%H:%M:%S")
            mDeviceID = m.group(2)
            numbeats = numbeats + 1
