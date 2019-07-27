














from marionette import MarionetteTestCase


class RenderedElementTests(MarionetteTestCase):

    def testWeCanGetComputedStyleValueOnElement(self):
        test_url = self.marionette.absolute_url('javascriptPage.html')
        self.marionette.navigate(test_url)
        element = self.marionette.find_element('id', "green-parent")
        backgroundColour = element.value_of_css_property("background-color")

        self.assertEqual("rgb(0, 128, 0)", backgroundColour)

        element = self.marionette.find_element('id', "red-item")
        backgroundColour = element.value_of_css_property("background-color")

        self.assertEqual("rgb(255, 0, 0)", backgroundColour)
