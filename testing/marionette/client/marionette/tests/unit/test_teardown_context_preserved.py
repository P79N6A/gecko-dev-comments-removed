



from marionette import MarionetteTestCase, SkipTest


class TestTearDownContext(MarionetteTestCase):
    def setUp(self):
        MarionetteTestCase.setUp(self)
        self.marionette.set_context(self.marionette.CONTEXT_CHROME)

    def tearDown(self):
        self.assertEqual(self.get_context(), self.marionette.CONTEXT_CHROME)
        MarionetteTestCase.tearDown(self)

    def get_context(self):
        return self.marionette._send_message('getContext', 'value')

    def test_skipped_teardown_ok(self):
        raise SkipTest("This should leave our teardown method in chrome context")
