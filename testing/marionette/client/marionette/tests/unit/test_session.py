



import marionette_test

class TestSession(marionette_test.MarionetteTestCase):
    def setUp(self):
        super(TestSession, self).setUp()
        self.marionette.delete_session()

    def test_new_session_returns_capabilities(self):
        
        caps = self.marionette.start_session()

        
        
        self.assertIsNotNone(self.marionette.session)

        
        self.assertIn("browserName", caps)
        self.assertIn("platformName", caps)
        self.assertIn("platformVersion", caps)

        
        self.assertIn("device", caps)
        self.assertIn("handlesAlerts", caps)
        self.assertIn("rotatable", caps)
        self.assertIn("takesScreenshot", caps)
        self.assertIn("version", caps)


