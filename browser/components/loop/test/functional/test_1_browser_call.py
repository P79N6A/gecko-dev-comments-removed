from marionette_test import MarionetteTestCase
from by import By
import urlparse
from marionette.errors import NoSuchElementException, StaleElementException

from marionette.wait import Wait
from time import sleep

import os
import sys
sys.path.insert(1, os.path.dirname(os.path.abspath(__file__)))

from serversetup import LoopTestServers
from config import *


class Test1BrowserCall(MarionetteTestCase):
    
    
    def setUp(self):
        
        self.loop_test_servers = LoopTestServers()

        MarionetteTestCase.setUp(self)

        
        
        
        self.marionette.enforce_gecko_prefs(FIREFOX_PREFERENCES)

        
        self.marionette.set_context("chrome")

    
    
    def wait_for_element_displayed(self, by, locator, timeout=None):
        Wait(self.marionette, timeout,
             ignored_exceptions=[NoSuchElementException, StaleElementException])\
            .until(lambda m: m.find_element(by, locator).is_displayed())
        return self.marionette.find_element(by, locator)

    
    def wait_for_element_exists(self, by, locator, timeout=None):
        Wait(self.marionette, timeout,
             ignored_exceptions=[NoSuchElementException, StaleElementException]) \
            .until(lambda m: m.find_element(by, locator))
        return self.marionette.find_element(by, locator)

    def switch_to_panel(self):
        button = self.marionette.find_element(By.ID, "loop-call-button")

        
        button.click()

        
        frame = self.marionette.find_element(By.ID, "loop")
        self.marionette.switch_to_frame(frame)

    def load_and_verify_standalone_ui(self, url):
        self.marionette.set_context("content")
        self.marionette.navigate(url)

        call_url_link = self.marionette.find_element(By.CLASS_NAME, "call-url") \
            .text
        self.assertEqual(url, call_url_link,
                         "should be on the correct page")

    def get_and_verify_call_url(self):
        
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
        return call_url

    def start_and_verify_outgoing_call(self):
        
        call_button = self.marionette.find_element(By.CLASS_NAME,
                                                   "btn-accept")
        call_button.click()

        
        pending_header = self.wait_for_element_displayed(By.CLASS_NAME,
                                                         "pending-header")
        self.assertEqual(pending_header.tag_name, "header",
                         "expect a pending header")

    def accept_and_verify_incoming_call(self):
        self.marionette.set_context("chrome")
        self.marionette.switch_to_frame()

        
        
        chatbox = self.wait_for_element_exists(By.TAG_NAME, 'chatbox')
        script = ("return document.getAnonymousElementByAttribute("
                  "arguments[0], 'class', 'chat-frame');")
        frame = self.marionette.execute_script(script, [chatbox])
        self.marionette.switch_to_frame(frame)

        
        call_button = self.marionette.find_element(By.CLASS_NAME,
                                                   "btn-accept")
        
        call_button.click()

        
        video = self.wait_for_element_displayed(By.CLASS_NAME, "media")
        self.assertEqual(video.tag_name, "div", "expect a video container")

    def hangup_call_and_verify_feedback(self):
        self.marionette.set_context("chrome")
        button = self.marionette.find_element(By.CLASS_NAME, "btn-hangup")

        
        
        
        
        
        sleep(2)
        button.click()

        
        feedback_form = self.wait_for_element_displayed(By.CLASS_NAME, "faces")
        self.assertEqual(feedback_form.tag_name, "div", "expect feedback form")

    def test_1_browser_call(self):
        self.switch_to_panel()

        call_url = self.get_and_verify_call_url()

        
        self.load_and_verify_standalone_ui(call_url)

        self.start_and_verify_outgoing_call()

        
        self.accept_and_verify_incoming_call()

        
        self.hangup_call_and_verify_feedback()

    def tearDown(self):
        self.loop_test_servers.shutdown()
        MarionetteTestCase.tearDown(self)
