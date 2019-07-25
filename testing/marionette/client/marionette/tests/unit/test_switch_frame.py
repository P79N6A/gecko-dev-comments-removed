


































import os
from marionette_test import MarionetteTestCase

class TestSwitchFrame(MarionetteTestCase):
    def test_switch_simple(self):
        self.assertTrue(self.marionette.execute_script("window.location.href = 'about:blank'; return true;"))
        self.assertEqual("about:blank", self.marionette.execute_script("return window.location.href;"))
        test_html = self.marionette.absolute_url("test_iframe.html")
        self.marionette.navigate(test_html)
        self.assertNotEqual("about:blank", self.marionette.execute_script("return window.location.href;"))
        self.assertEqual("Marionette IFrame Test", self.marionette.execute_script("return window.document.title;"))
        self.marionette.switch_to_frame("test_iframe")
        self.assertTrue("test.html" in self.marionette.get_url())
