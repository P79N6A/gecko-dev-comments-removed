


from luciddream import LucidDreamTestCase

class TestSample(LucidDreamTestCase):
    def test_sample(self):
        
        self.assertIsNotNone(self.marionette.session)
        self.assertIsNotNone(self.browser.session)

    def test_js(self):
        'Test that we can run a JavaScript test in both Marionette instances'
        self.run_js_test('test.js', self.marionette)
        self.run_js_test('test.js', self.browser)
