



from marionette_test import MarionetteTestCase


class TestGetAttribute(MarionetteTestCase):
    def test_getAttribute(self):
        test_html = self.marionette.absolute_url("test.html")
        self.marionette.navigate(test_html)
        l = self.marionette.find_element("id", "mozLink")
        self.assertEqual("mozLink", l.get_attribute("id"))
