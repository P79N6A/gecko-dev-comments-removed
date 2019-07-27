from marionette_test import MarionetteTestCase
from by import By
import urlparse
from errors import NoSuchElementException, StaleElementException

from wait import Wait
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
        button = self.marionette.find_element(By.ID, "loop-button")

        
        button.click()

        
        frame = self.marionette.find_element(By.ID, "loop")
        self.marionette.switch_to_frame(frame)

    def load_and_verify_standalone_ui(self, url):
        self.marionette.set_context("content")
        self.marionette.navigate(url)

    def start_a_conversation(self):
        
        sleep(2)
        button = self.marionette.find_element(By.CSS_SELECTOR, ".rooms .btn-info")

        
        button.click()

    def get_and_verify_call_url(self):
        
        self.start_a_conversation()

        
        sleep(2)
        call_url = self.marionette.find_element(By.CLASS_NAME, \
                                                "room-url-link").text

        self.assertIn(urlparse.urlparse(call_url).scheme, ['http', 'https'],
                      "call URL returned by server " + call_url +
                      " has invalid scheme")
        return call_url

    def start_and_verify_outgoing_call(self):
        
        sleep(2)
        
        call_button = self.marionette.find_element(By.CLASS_NAME,
                                                   "btn-join")
        call_button.click()

    def accept_and_verify_incoming_call(self):
        self.marionette.set_context("chrome")
        self.marionette.switch_to_frame()

        
        
        chatbox = self.wait_for_element_exists(By.TAG_NAME, 'chatbox')
        script = ("return document.getAnonymousElementByAttribute("
                  "arguments[0], 'class', 'chat-frame');")
        frame = self.marionette.execute_script(script, [chatbox])
        self.marionette.switch_to_frame(frame)

        
        video = self.wait_for_element_displayed(By.CLASS_NAME, "media")
        self.assertEqual(video.tag_name, "div", "expect a video container")

    def hangup_call_and_verify_feedback(self):
        self.marionette.set_context("chrome")
        button = self.marionette.find_element(By.CLASS_NAME, "btn-hangup")

        
        
        
        
        
        sleep(5)
        button.click()

        
        feedback_form = self.wait_for_element_displayed(By.CLASS_NAME, "faces")
        self.assertEqual(feedback_form.tag_name, "div", "expect feedback form")

    def test_1_browser_call(self):
        self.switch_to_panel()

        call_url = self.get_and_verify_call_url()

        
        self.load_and_verify_standalone_ui(call_url)

        self.start_and_verify_outgoing_call()

        
        self.accept_and_verify_incoming_call()

        
        
        sleep(5)

        
        self.hangup_call_and_verify_feedback()

    def tearDown(self):
        self.loop_test_servers.shutdown()
        MarionetteTestCase.tearDown(self)
