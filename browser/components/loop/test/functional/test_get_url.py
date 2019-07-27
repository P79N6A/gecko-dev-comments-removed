from marionette_test import MarionetteTestCase
from marionette.errors import NoSuchElementException
from marionette.errors import StaleElementException
from by import By

from marionette.wait import Wait
import os


from mozprocess import processhandler
import urlparse





import sys
sys.path.append(os.path.dirname(__file__))
import hanging_threads

CONTENT_SERVER_PORT = 3001
LOOP_SERVER_PORT = 5001

CONTENT_SERVER_COMMAND = ["make", "runserver"]
CONTENT_SERVER_ENV = os.environ.copy()


CONTENT_SERVER_ENV.update({"PORT": str(CONTENT_SERVER_PORT),
                           "LOOP_SERVER_PORT": str(LOOP_SERVER_PORT)})

WEB_APP_URL = "http://localhost:" + str(CONTENT_SERVER_PORT) + \
              "/content/#call/{token}"

LOOP_SERVER_COMMAND = ["make", "runserver"]
LOOP_SERVER_ENV = os.environ.copy()


LOOP_SERVER_ENV.update({"NODE_ENV": "dev", "PORT": str(LOOP_SERVER_PORT),
                        "WEB_APP_URL": WEB_APP_URL})


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


class TestGetUrl(MarionetteTestCase):
    
    
    
    def setUp(self):
        
        self.loop_test_servers = LoopTestServers()

        MarionetteTestCase.setUp(self)

        
        
        
        preferences = {"loop.server": "http://localhost:" + str(LOOP_SERVER_PORT)}
        self.marionette.enforce_gecko_prefs(preferences)

        
        self.marionette.set_context("chrome")

    def switch_to_panel(self):
        button = self.marionette.find_element(By.ID, "loop-call-button")

        
        button.click()

        
        frame = self.marionette.find_element(By.ID, "loop")
        self.marionette.switch_to_frame(frame)

    
    
    
    def wait_for_element_displayed(self, by, locator, timeout=None):
        Wait(self.marionette, timeout,
             ignored_exceptions=[NoSuchElementException, StaleElementException])\
            .until(lambda m: m.find_element(by, locator).is_displayed())
        return self.marionette.find_element(by, locator)

    def test_get_url(self):
        self.switch_to_panel()

        
        url_input_element = self.wait_for_element_displayed(By.TAG_NAME,
                                                            "input")

        
        self.assertEqual(url_input_element.get_attribute("class"), "pending",
                         "expect the input to be pending")

        
        
        
        
        
        
        
        url_input_element = self.wait_for_element_displayed(By.CLASS_NAME,
                                                            "callUrl")
        call_url = url_input_element.get_attribute("value")
        self.assertNotEqual(call_url, u'',
                            "input is populated with call URL after pending"
                            " is finished")

        self.assertIn(urlparse.urlparse(call_url).scheme, ['http', 'https'],
                      "call URL returned by server " + call_url +
                      " has invalid scheme")

    def tearDown(self):
        self.loop_test_servers.shutdown()
        MarionetteTestCase.tearDown(self)
