




import socket
from threading import Thread
import unittest
import time

class MockAgent(object):
    def __init__(self, tester, start_commands = None, commands = []):
        if start_commands:
            self.commands = start_commands
        else:
            self.commands = [("testroot", "/mnt/sdcard"),
                                   ("isdir /mnt/sdcard/tests", "TRUE"),
                                   ("ver", "SUTAgentAndroid Version 1.14")]
        self.commands = self.commands + commands

        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.bind(("127.0.0.1", 0))
        self._sock.listen(1)

        self.tester = tester

        self.thread = Thread(target=self._serve_thread)
        self.thread.start()

    @property
    def port(self):
        return self._sock.getsockname()[1]

    def _serve_thread(self):
        conn = None
        while self.commands:
            if not conn:
                conn, addr = self._sock.accept()
                conn.send("$>\x00")
            (command, response) = self.commands.pop(0)
            data = conn.recv(1024).strip()
            self.tester.assertEqual(data, command)
            
            
            
            if response is None:
                conn.shutdown(socket.SHUT_RDWR)
                conn.close()
                conn = None
            elif type(response) is int:
                time.sleep(response)
            else:
                
                
                if "pull" in command:
                    conn.send(response)
                else:
                    conn.send("%s\n" % response)
                conn.send("$>\x00")

    def wait(self):
        self.thread.join()
