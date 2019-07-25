


































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

    def test_switch_nested(self):
        self.assertTrue(self.marionette.execute_script("window.location.href = 'about:blank'; return true;"))
        self.assertEqual("about:blank", self.marionette.execute_script("return window.location.href;"))
        test_html = self.marionette.absolute_url("test_nested_iframe.html")
        self.marionette.navigate(test_html)
        self.assertNotEqual("about:blank", self.marionette.execute_script("return window.location.href;"))
        self.assertEqual("Marionette IFrame Test", self.marionette.execute_script("return window.document.title;"))
        self.marionette.switch_to_frame("test_iframe")
        self.assertTrue("test_inner_iframe.html" in self.marionette.get_url())
        self.marionette.switch_to_frame("inner_frame")
        self.assertTrue("test.html" in self.marionette.get_url())
        self.marionette.switch_to_frame() 
        self.assertTrue("test_nested_iframe.html" in self.marionette.get_url())
        
        self.assertTrue("test_nested_iframe.html" in self.marionette.execute_script("return window.location.href;"))

class TestSwitchFrameChrome(MarionetteTestCase):
    def setUp(self):
        MarionetteTestCase.setUp(self)
        self.marionette.set_context("chrome")
        self.win = self.marionette.get_window()
        
        unit = os.path.abspath(os.path.join(os.path.realpath(__file__), os.path.pardir))
        tests = os.path.abspath(os.path.join(unit, os.path.pardir))
        mpath = os.path.abspath(os.path.join(tests, os.path.pardir))
        xul = "file://" + os.path.join(mpath, "www", "test.xul")
        self.marionette.execute_script("window.open('" + xul +"', '_blank', 'chrome,centerscreen');")

    def tearDown(self):
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
        
    
    def test_switch_nested(self):
        pass
