



from marionette_test import MarionetteTestCase

class testNavigateToDefault(MarionetteTestCase):
    def setUp(self):
        MarionetteTestCase.setUp(self)
        
        
        self.marionette.timeouts(self.marionette.TIMEOUT_PAGE, 90000)

    def test_navigate_to_default_url(self):
        try:
            self.marionette.navigate("app://system.gaiamobile.org/index.html")
        except:
            self.assertTrue(False, "Can not navigate to system app.")
