



import os
from marionette_test import MarionetteTestCase
from errors import JavascriptException


def switch_to_window_verify(test, start_url, frame, verify_title, verify_url):
    test.assertTrue(test.marionette.execute_script("window.location.href = 'about:blank'; return true;"))
    test.assertEqual("about:blank", test.marionette.execute_script("return window.location.href;"))
    test_html = test.marionette.absolute_url(start_url)
    test.marionette.navigate(test_html)
    test.assertNotEqual("about:blank", test.marionette.execute_script("return window.location.href;"))
    test.assertEqual(verify_title, test.marionette.title)
    test.marionette.switch_to_frame(frame)
    test.assertTrue(verify_url in test.marionette.get_url())

class TestSwitchFrame(MarionetteTestCase):
    def test_switch_simple(self):
        switch_to_window_verify(self, "test_iframe.html", "test_iframe", "Marionette IFrame Test", "test.html")

    def test_switch_nested(self):
        switch_to_window_verify(self, "test_nested_iframe.html", "test_iframe", "Marionette IFrame Test", "test_inner_iframe.html")
        self.marionette.switch_to_frame("inner_frame")
        self.assertTrue("test.html" in self.marionette.get_url())
        self.marionette.switch_to_frame() 
        self.assertTrue("test_nested_iframe.html" in self.marionette.get_url())
        
        self.assertTrue("test_nested_iframe.html" in self.marionette.execute_script("return window.location.href;"))

    def test_stack_trace(self):
        switch_to_window_verify(self, "test_iframe.html", "test_iframe", "Marionette IFrame Test", "test.html")
        with self.assertRaises(JavascriptException) as cm:
            self.marionette.execute_async_script("foo();")
        self.assertTrue("foo" in cm.exception.msg)

class TestSwitchFrameChrome(MarionetteTestCase):
    def setUp(self):
        MarionetteTestCase.setUp(self)
        self.marionette.set_context("chrome")
        self.win = self.marionette.current_window_handle
        self.marionette.execute_script("window.open('chrome://marionette/content/test.xul', 'foo', 'chrome,centerscreen');")
        self.marionette.switch_to_window('foo')
        self.assertNotEqual(self.win, self.marionette.current_window_handle)

    def tearDown(self):
        self.assertNotEqual(self.win, self.marionette.current_window_handle)
        self.marionette.execute_script("window.close();")
        self.marionette.switch_to_window(self.win)
        MarionetteTestCase.tearDown(self)

    def test_switch_simple(self):
        self.assertTrue("test.xul" in self.marionette.get_url())
        self.marionette.switch_to_frame(0)
        self.assertTrue("test2.xul" in self.marionette.get_url())
        self.marionette.switch_to_frame()
        self.assertTrue("test.xul" in self.marionette.get_url())
        self.marionette.switch_to_frame("iframe")
        self.assertTrue("test2.xul" in self.marionette.get_url())
        self.marionette.switch_to_frame()
        self.assertTrue("test.xul" in self.marionette.get_url())
        self.marionette.switch_to_frame("iframename")
        self.assertTrue("test2.xul" in self.marionette.get_url())
        self.marionette.switch_to_frame()
        self.assertTrue("test.xul" in self.marionette.get_url())
        
    def test_stack_trace(self):
        self.assertTrue("test.xul" in self.marionette.get_url())
        self.marionette.switch_to_frame(0)
        with self.assertRaises(JavascriptException) as cm:
            self.marionette.execute_async_script("foo();")
        self.assertTrue("foo" in cm.exception.msg)
