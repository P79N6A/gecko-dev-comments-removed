



from marionette import MarionetteTestCase

from marionette_driver.by import By
from marionette_driver.errors import NoSuchElementException
from marionette_driver.wait import Wait


class TestWindowSwitching(MarionetteTestCase):
    def testJSWindowCreationAndSwitching(self):
        test_html = self.marionette.absolute_url("test_windows.html")
        self.marionette.navigate(test_html)

        self.current_window = self.marionette.current_window_handle
        link = self.marionette.find_element("link text", "Open new window")
        link.click()

        windows = self.marionette.window_handles
        windows.remove(self.current_window)
        self.marionette.switch_to_window(windows[0])

        title = self.marionette.execute_script("return document.title")
        results_page = self.marionette.absolute_url("resultPage.html")
        self.assertEqual(self.marionette.get_url(), results_page)
        self.assertEqual(title, "We Arrive Here")

        
        other_page = self.marionette.absolute_url("test.html")
        self.marionette.navigate(other_page)
        other_window = self.marionette.current_window_handle

        
        
        Wait(self.marionette, timeout=30, ignored_exceptions=NoSuchElementException).until(
            lambda m: m.find_element(By.ID, 'mozLink'))

        self.assertEqual(other_window, self.marionette.current_window_handle)
        self.marionette.switch_to_window(self.current_window)
        self.assertEqual(self.current_window, self.marionette.current_window_handle)

    def tearDown(self):
        window_handles = self.marionette.window_handles
        window_handles.remove(self.current_window)
        for handle in window_handles:
            self.marionette.switch_to_window(handle)
            self.marionette.close()
