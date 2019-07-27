














from marionette_driver import By
from marionette_driver.application_cache import ApplicationCache
from marionette_driver.errors import NoSuchElementException
from marionette import MarionetteTestCase


class ChromeTests(MarionetteTestCase):

    def test_hang_until_timeout(self):
        with self.marionette.using_context('chrome'):
            current_handle = self.marionette.current_chrome_window_handle
            menu = self.marionette.find_element(By.ID, 'aboutName')
            menu.click()
            handles = self.marionette.chrome_window_handles
            handles.remove(current_handle)
            self.marionette.switch_to_window(handles[0])
            self.assertRaises(NoSuchElementException, self.marionette.find_element, By.ID, 'dek')
