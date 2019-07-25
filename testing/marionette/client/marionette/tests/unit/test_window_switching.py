



import os
import time
from marionette_test import MarionetteTestCase

class TestWindowSwitching(MarionetteTestCase):
    def testJSWindowCreationAndSwitching(self):
        test_html = self.marionette.absolute_url("test_windows.html")
        self.marionette.navigate(test_html)

        current_window = self.marionette.get_window()
        link = self.marionette.find_element("link text", "Open new window")
        link.click()

        windows = self.marionette.get_windows()
        windows.remove(current_window)
        self.marionette.switch_to_window(windows[0])

        title = self.marionette.execute_script("return document.title")
        results_page = self.marionette.absolute_url("resultPage.html")
        self.assertEqual(self.marionette.get_url(), results_page)
        self.assertEqual(title, "We Arrive Here")

        
        other_page = self.marionette.absolute_url("test.html")
        self.marionette.navigate(other_page)
        other_window = self.marionette.get_window()

        
        
        for i in range(30):
            try:
                self.marionette.find_element("id", "mozLink")
                break
            except:
                pass
            time.sleep(1)

        self.assertEqual(other_window, self.marionette.get_window())
        self.marionette.switch_to_window(current_window)
        self.assertEqual(current_window, self.marionette.get_window())

