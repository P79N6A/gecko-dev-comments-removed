from marionette_driver.by import By
from marionette_driver.errors import NoSuchElementException, StaleElementException

from marionette_driver import Wait
from marionette import MarionetteTestCase

import os
import sys
import urlparse
sys.path.insert(1, os.path.dirname(os.path.abspath(__file__)))

import pyperclip

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

    def wait_for_subelement_displayed(self, parent, by, locator, timeout=None):
        Wait(self.marionette, timeout,
             ignored_exceptions=[NoSuchElementException, StaleElementException])\
            .until(lambda m: parent.find_element(by, locator).is_displayed())
        return parent.find_element(by, locator)

    
    def wait_for_element_exists(self, by, locator, timeout=None):
        Wait(self.marionette, timeout,
             ignored_exceptions=[NoSuchElementException, StaleElementException]) \
            .until(lambda m: m.find_element(by, locator))
        return self.marionette.find_element(by, locator)

    def wait_for_element_enabled(self, element, timeout=10):
        Wait(self.marionette, timeout) \
            .until(lambda e: element.is_enabled(),
                   message="Timed out waiting for element to be enabled")

    def wait_for_element_attribute_to_be_false(self, element, attribute, timeout=10):
        Wait(self.marionette, timeout) \
            .until(lambda e: element.get_attribute(attribute) == "false",
                   message="Timeout out waiting for " + attribute + " to be false")

    def switch_to_panel(self):
        button = self.marionette.find_element(By.ID, "loop-button")

        
        button.click()

        
        frame = self.marionette.find_element(By.ID, "loop-panel-iframe")
        self.marionette.switch_to_frame(frame)

    def switch_to_chatbox(self):
        self.marionette.set_context("chrome")
        self.marionette.switch_to_frame()

        
        
        chatbox = self.wait_for_element_exists(By.TAG_NAME, 'chatbox')
        script = ("return document.getAnonymousElementByAttribute("
                  "arguments[0], 'class', 'chat-frame');")
        frame = self.marionette.execute_script(script, [chatbox])
        self.marionette.switch_to_frame(frame)

    def switch_to_standalone(self):
        self.marionette.set_context("content")

    def local_start_a_conversation(self):
        button = self.marionette.find_element(By.CSS_SELECTOR, ".rooms .btn-info")

        self.wait_for_element_enabled(button, 120)

        button.click()

    def local_check_room_self_video(self):
        self.switch_to_chatbox()

        
        media_container = self.wait_for_element_displayed(By.CLASS_NAME, "media")
        self.assertEqual(media_container.tag_name, "div", "expect a video container")

        self.check_video(".local .OT_publisher .OT_widget-container");

    def local_get_and_verify_room_url(self):
        self.switch_to_chatbox()
        button = self.wait_for_element_displayed(By.CLASS_NAME, "btn-copy")

        button.click()

        
        room_url = pyperclip.paste()

        self.assertIn(urlparse.urlparse(room_url).scheme, ['http', 'https'],
                      "room URL returned by server: '" + room_url +
                      "' has invalid scheme")
        return room_url

    def standalone_load_and_join_room(self, url):
        self.switch_to_standalone()
        self.marionette.navigate(url)

        
        join_button = self.wait_for_element_displayed(By.CLASS_NAME,
                                                      "btn-join")
        join_button.click()

    
    def check_video(self, selector):
        video_wrapper = self.wait_for_element_displayed(By.CSS_SELECTOR,
                                                        selector, 20)
        video = self.wait_for_subelement_displayed(video_wrapper,
                                                   By.TAG_NAME, "video")

        self.wait_for_element_attribute_to_be_false(video, "paused")
        self.assertEqual(video.get_attribute("ended"), "false")

    def standalone_check_remote_video(self):
        self.switch_to_standalone()
        self.check_video(".remote .OT_subscriber .OT_widget-container")

    def local_check_remote_video(self):
        self.switch_to_chatbox()
        self.check_video(".remote .OT_subscriber .OT_widget-container")

    def local_enable_screenshare(self):
        self.switch_to_chatbox()
        button = self.marionette.find_element(By.CLASS_NAME, "btn-screen-share")

        button.click()

    def standalone_check_remote_screenshare(self):
        self.switch_to_standalone()
        self.check_video(".media .screen .OT_subscriber .OT_widget-container")

    def remote_leave_room_and_verify_feedback(self):
        self.switch_to_standalone()
        button = self.marionette.find_element(By.CLASS_NAME, "btn-hangup")

        button.click()

        
        feedback_form = self.wait_for_element_displayed(By.CLASS_NAME, "faces")
        self.assertEqual(feedback_form.tag_name, "div", "expect feedback form")

        self.switch_to_chatbox()
        
        self.wait_for_element_displayed(By.CLASS_NAME, "room-invitation-content")

    def local_get_chatbox_window_expr(self, expr):
        """
        :expr: a sub-expression which must begin with a property of the
        global content window (e.g. "location.path")

        :return: the value of the given sub-expression as evaluated in the
        chatbox content window
        """
        self.marionette.set_context("chrome")
        self.marionette.switch_to_frame()

        
        
        chatbox = self.wait_for_element_exists(By.TAG_NAME, 'chatbox')
        script = '''
            let chatBrowser = document.getAnonymousElementByAttribute(
              arguments[0], 'class',
              'chat-frame')

            // note that using wrappedJSObject waives X-ray vision, which
            // has security implications, but because we trust the code
            // running in the chatbox, it should be reasonably safe
            let chatGlobal = chatBrowser.contentWindow.wrappedJSObject;

            return chatGlobal.''' + expr

        return self.marionette.execute_script(script, [chatbox])

    def local_get_media_start_time(self):
        return self.local_get_chatbox_window_expr(
            "loop.conversation._sdkDriver._getTwoWayMediaStartTime()")

    
    def local_get_media_start_time_uninitialized(self):
        return self.local_get_chatbox_window_expr(
            "loop.conversation._sdkDriver.CONNECTION_START_TIME_UNINITIALIZED"
        )

    def local_check_media_start_time_uninitialized(self):
        self.assertEqual(
            self.local_get_media_start_time(),
            self.local_get_media_start_time_uninitialized(),
            "media start time should be uninitialized before "
            "link clicker enters room")

    def local_check_media_start_time_initialized(self):
        self.assertNotEqual(
            self.local_get_media_start_time(),
            self.local_get_media_start_time_uninitialized(),
            "media start time should be initialized after "
            "media is bidirectionally connected")

    def local_check_connection_length_noted(self):
        noted_calls = self.local_get_chatbox_window_expr(
            "loop.conversation._sdkDriver._connectionLengthNotedCalls")

        self.assertGreater(noted_calls, 0,
                           "OTSdkDriver._connectionLengthNotedCalls should be "
                           "> 0, noted_calls = " + str(noted_calls))

    def test_1_browser_call(self):
        self.switch_to_panel()

        self.local_start_a_conversation()

        
        self.local_check_room_self_video()

        
        self.local_check_media_start_time_uninitialized()

        room_url = self.local_get_and_verify_room_url()

        
        self.standalone_load_and_join_room(room_url)

        
        self.standalone_check_remote_video()
        self.local_check_remote_video()

        
        
        self.local_check_media_start_time_initialized()

        
        
        
        

        
        
        
        
        
        self.remote_leave_room_and_verify_feedback()

        self.local_check_connection_length_noted()

    def tearDown(self):
        self.loop_test_servers.shutdown()
        MarionetteTestCase.tearDown(self)
