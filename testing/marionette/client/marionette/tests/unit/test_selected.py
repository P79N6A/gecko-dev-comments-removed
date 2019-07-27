



from marionette import MarionetteTestCase


class TestSelected(MarionetteTestCase):
    def test_selected(self):
        test_html = self.marionette.absolute_url("test.html")
        self.marionette.navigate(test_html)
        box = self.marionette.find_element("name", "myCheckBox")
        self.assertFalse(box.is_selected())
        box.click()
        self.assertTrue(box.is_selected())
