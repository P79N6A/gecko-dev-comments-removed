





from mozprocess import processhandler





import sys
import os
sys.path.append(os.path.dirname(__file__))
import hanging_threads
from config import *

CONTENT_SERVER_COMMAND = ["make", "runserver"]
CONTENT_SERVER_ENV = os.environ.copy()


CONTENT_SERVER_ENV.update({"PORT": str(CONTENT_SERVER_PORT),
                           "LOOP_SERVER_PORT": str(LOOP_SERVER_PORT)})

ROOMS_WEB_APP_URL = "http://localhost:" + str(CONTENT_SERVER_PORT) + \
  "/content/{token}"

LOOP_SERVER_COMMAND = ["make", "runserver"]
LOOP_SERVER_ENV = os.environ.copy()


LOOP_SERVER_ENV.update({"NODE_ENV": "dev",
                        "PORT": str(LOOP_SERVER_PORT),
                        "SERVER_ADDRESS": "localhost:" + str(LOOP_SERVER_PORT),
                        "ROOMS_WEB_APP_URL": ROOMS_WEB_APP_URL})


class LoopTestServers:
    def __init__(self):
        self.loop_server = self.start_loop_server()
        self.content_server = self.start_content_server()

    @staticmethod
    def start_loop_server():
        loop_server_location = os.environ.get('LOOP_SERVER')
        if loop_server_location is None:
            raise Exception('LOOP_SERVER variable not set')

        os.chdir(loop_server_location)

        p = processhandler.ProcessHandler(LOOP_SERVER_COMMAND,
                                          env=LOOP_SERVER_ENV)
        p.run()
        return p

    @staticmethod
    def start_content_server():
        content_server_location = os.environ.get('STANDALONE_SERVER')
        if content_server_location is None:
          content_server_location = os.path.join(os.path.dirname(__file__),
                                                 "../../standalone")
        os.chdir(content_server_location)

        p = processhandler.ProcessHandler(CONTENT_SERVER_COMMAND,
                                          env=CONTENT_SERVER_ENV)
        p.run()
        return p

    def shutdown(self):
        self.content_server.kill()
        self.loop_server.kill()

