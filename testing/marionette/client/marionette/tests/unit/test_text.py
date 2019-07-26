



from marionette_test import MarionetteTestCase


class TestText(MarionetteTestCase):
    def test_getText(self):
        test_html = self.marionette.absolute_url("test.html")
        self.marionette.navigate(test_html)
        l = self.marionette.find_element("id", "mozLink")
        self.assertEqual("Click me!", l.text)

    def test_clearText(self):
        test_html = self.marionette.absolute_url("test.html")
        self.marionette.navigate(test_html)
        l = self.marionette.find_element("name", "myInput")
        self.assertEqual("asdf", self.marionette.execute_script("return arguments[0].value;", [l]))
        l.clear()
        self.assertEqual("", self.marionette.execute_script("return arguments[0].value;", [l]))

    def test_sendKeys(self):
        test_html = self.marionette.absolute_url("test.html")
        self.marionette.navigate(test_html)
        l = self.marionette.find_element("name", "myInput")
        self.assertEqual("asdf", self.marionette.execute_script("return arguments[0].value;", [l]))
        l.send_keys("o")
        self.assertEqual("asdfo", self.marionette.execute_script("return arguments[0].value;", [l]))
