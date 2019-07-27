



from by import By
from errors import NoSuchElementException, ElementNotVisibleException
from marionette_test import MarionetteTestCase
from wait import Wait


class TestClick(MarionetteTestCase):
    def test_click(self):
        test_html = self.marionette.absolute_url("test.html")
        self.marionette.navigate(test_html)
        link = self.marionette.find_element(By.ID, "mozLink")
        link.click()
        self.assertEqual("Clicked", self.marionette.execute_script("return document.getElementById('mozLink').innerHTML;"))

    def test_clicking_a_link_made_up_of_numbers_is_handled_correctly(self):
        test_html = self.marionette.absolute_url("clicks.html")
        self.marionette.navigate(test_html)
        self.marionette.find_element(By.LINK_TEXT, "333333").click()
        Wait(self.marionette, timeout=30, ignored_exceptions=NoSuchElementException).until(
            lambda m: m.find_element(By.ID, 'username'))
        self.assertEqual(self.marionette.title, "XHTML Test Page")

    def test_clicking_an_element_that_is_not_displayed_raises(self):
        test_html = self.marionette.absolute_url('hidden.html')
        self.marionette.navigate(test_html)

        with self.assertRaises(ElementNotVisibleException):
            self.marionette.find_element(By.ID, 'child').click()
