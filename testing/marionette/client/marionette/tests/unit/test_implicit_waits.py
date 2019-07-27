



from marionette import MarionetteTestCase
from marionette_driver.errors import NoSuchElementException

class TestImplicitWaits(MarionetteTestCase):
    def testShouldImplicitlyWaitForASingleElement(self):
        test_html = self.marionette.absolute_url("test_dynamic.html")
        self.marionette.navigate(test_html)
        add = self.marionette.find_element("id", "adder")
        self.marionette.set_search_timeout("30000")
        add.click()
        
        self.marionette.find_element("id", "box0")

    def testShouldStillFailToFindAnElementWhenImplicitWaitsAreEnabled(self):
        test_html = self.marionette.absolute_url("test_dynamic.html")
        self.marionette.navigate(test_html)
        self.marionette.set_search_timeout("3000")
        try:
            self.marionette.find_element("id", "box0")
            self.fail("Should have thrown a a NoSuchElementException")
        except NoSuchElementException:
            pass
        except Exception:
            self.fail("Should have thrown a NoSuchElementException")
