



import re
import socket

from dmunit import DeviceManagerTestCase


class PromptTestCase(DeviceManagerTestCase):

    def tearDown(self):
        if self.sock:
            self.sock.close()

    def runTest(self):
        """This tests getting a prompt from the device.
        """
        self.sock = None
        ip = self.dm.host
        port = self.dm.port

        promptre = re.compile('.*\$\>\x00')
        data = ""
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((ip, int(port)))
        data = self.sock.recv(1024)
        print data
        self.assert_(promptre.match(data))
