



from marionette_test import MarionetteTestCase

class testNavigateToDefault(MarionetteTestCase):
    def test_navigate_to_default_url(self):
        try:
            self.marionette.navigate("app://system.gaiamobile.org/index.html")
        except:
            self.assertTrue(Flase, "Can not navigate to system app.")
