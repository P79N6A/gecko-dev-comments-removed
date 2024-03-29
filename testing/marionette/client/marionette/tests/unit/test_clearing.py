



from marionette import MarionetteTestCase
from marionette_driver.errors import InvalidElementStateException

class TestClear(MarionetteTestCase):
    def testWriteableTextInputShouldClear(self):
        test_html = self.marionette.absolute_url("test_clearing.html")
        self.marionette.navigate(test_html)
        element = self.marionette.find_element("id", "writableTextInput")
        element.clear()
        self.assertEqual("", element.get_attribute("value"))

    def testTextInputShouldNotClearWhenReadOnly(self):
        test_html = self.marionette.absolute_url("test_clearing.html")
        self.marionette.navigate(test_html)
        element = self.marionette.find_element("id","readOnlyTextInput")
        try:
            element.clear()
            self.fail("Should not have been able to clear")
        except InvalidElementStateException:
            pass

    def testWritableTextAreaShouldClear(self):
        test_html = self.marionette.absolute_url("test_clearing.html")
        self.marionette.navigate(test_html)
        element = self.marionette.find_element("id","writableTextArea")
        element.clear()
        self.assertEqual("", element.get_attribute("value"))

    def testTextAreaShouldNotClearWhenDisabled(self):
        test_html = self.marionette.absolute_url("test_clearing.html")
        self.marionette.navigate(test_html)
        element = self.marionette.find_element("id","textAreaNotenabled")
        try:
            element.clear()
            self.fail("Should not have been able to clear")
        except InvalidElementStateException:
            pass

    def testTextAreaShouldNotClearWhenReadOnly(self):
        test_html = self.marionette.absolute_url("test_clearing.html")
        self.marionette.navigate(test_html)
        element = self.marionette.find_element("id","textAreaReadOnly")
        try:
            element.clear()
            self.fail("Should not have been able to clear")
        except InvalidElementStateException:
            pass

    def testContentEditableAreaShouldClear(self):
        test_html = self.marionette.absolute_url("test_clearing.html")
        self.marionette.navigate(test_html)
        element = self.marionette.find_element("id","content-editable")
        element.clear()
        self.assertEqual("", element.text)

    def testTextInputShouldNotClearWhenDisabled(self):
        test_html = self.marionette.absolute_url("test_clearing.html")
        self.marionette.navigate(test_html)
        try:
            element = self.marionette.find_element("id","textInputnotenabled")
            self.assertFalse(element.is_enabled())
            element.clear()
            self.fail("Should not have been able to clear")
        except InvalidElementStateException:
            pass
