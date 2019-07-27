



from marionette import MarionetteTestCase

class TestSession(MarionetteTestCase):
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
        self.assertIn("rotatable", caps)
        self.assertIn("takesScreenshot", caps)
        self.assertIn("version", caps)

    def test_we_can_get_the_session_id(self):
        
        caps = self.marionette.start_session()

        self.assertTrue(self.marionette.session_id is not None)
        self.assertTrue(isinstance(self.marionette.session_id, unicode))

    def test_we_can_set_the_session_id(self):
        
        caps = self.marionette.start_session(session_id="ILoveCheese")

        self.assertEqual(self.marionette.session_id, "ILoveCheese")
        self.assertTrue(isinstance(self.marionette.session_id, unicode))
