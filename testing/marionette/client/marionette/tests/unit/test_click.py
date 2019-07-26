



import time
from marionette_test import MarionetteTestCase
from by import By


class TestClick(MarionetteTestCase):
    def test_click(self):
        test_html = self.marionette.absolute_url("test.html")
        self.marionette.navigate(test_html)
        link = self.marionette.find_element(By.ID, "mozLink")
        link.click()
        self.assertEqual("Clicked", self.marionette.execute_script("return document.getElementById('mozLink').innerHTML;"))

    def testClickingALinkMadeUpOfNumbersIsHandledCorrectly(self):
        test_html = self.marionette.absolute_url("clicks.html")
        self.marionette.navigate(test_html)
        self.marionette.find_element(By.LINK_TEXT, "333333").click()
        count = 0
        while len(self.marionette.find_elements(By.ID, "username")) == 0:
            count += 1
            time.sleep(1)
            if count == 30:
                self.fail("Element id=username not found after 30 seconds")

        self.assertEqual(self.marionette.title, "XHTML Test Page")


class TestClickChrome(MarionetteTestCase):
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

    def test_click(self):
        wins = self.marionette.window_handles
        wins.remove(self.win)
        newWin = wins.pop()
        self.marionette.switch_to_window(newWin)
        box = self.marionette.find_element(By.ID, "testBox")
        self.assertFalse(self.marionette.execute_script("return arguments[0].checked;", [box]))
        box.click()
        self.assertTrue(self.marionette.execute_script("return arguments[0].checked;", [box]))
