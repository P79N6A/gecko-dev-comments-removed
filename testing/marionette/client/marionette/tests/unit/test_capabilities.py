



from marionette_test import MarionetteTestCase

class TestCapabilities(MarionetteTestCase):

    def testThatWeCanGetTheCapabilities(self):
        capabilities = self.marionette.session_capabilities
        self.assertTrue(capabilities.has_key('takesScreenshot'))

