


from marionette import MarionetteTestCase

class TestRunJSTest(MarionetteTestCase):
    def test_basic(self):
        self.run_js_test('test_simpletest_pass.js')
        self.run_js_test('test_simpletest_fail.js')
