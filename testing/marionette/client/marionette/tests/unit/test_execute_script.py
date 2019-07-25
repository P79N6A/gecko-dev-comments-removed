


































from marionette_test import MarionetteTestCase
from errors import JavascriptException, MarionetteException

class TestExecuteContent(MarionetteTestCase):
    def test_execute_simple(self):
        self.assertEqual(1, self.marionette.execute_script("return 1;"))

    def test_check_window(self):
        self.assertTrue(self.marionette.execute_script("return (window !=null && window != undefined);"))

    def test_execute_no_return(self):
        self.assertEqual(self.marionette.execute_script("1;"), None)

    def test_execute_js_exception(self):
        self.assertRaises(JavascriptException,
            self.marionette.execute_script, "return foo(bar);")

    def test_execute_permission(self):
        self.assertRaises(JavascriptException,
                          self.marionette.execute_script,
                          "return Components.classes;")

    def test_complex_return_values(self):
        self.assertEqual(self.marionette.execute_script("return [1, 2];"), [1, 2])
        self.assertEqual(self.marionette.execute_script("return {'foo': 'bar', 'fizz': 'fazz'};"),
                         {'foo': 'bar', 'fizz': 'fazz'})
        self.assertEqual(self.marionette.execute_script("return [1, {'foo': 'bar'}, 2];"),
                         [1, {'foo': 'bar'}, 2])
        self.assertEqual(self.marionette.execute_script("return {'foo': [1, 'a', 2]};"),
                         {'foo': [1, 'a', 2]})


class TestExecuteChrome(TestExecuteContent):
    def setUp(self):
        super(TestExecuteChrome, self).setUp()
        self.marionette.set_context("chrome")

    def test_execute_permission(self):
        self.assertEqual(1, self.marionette.execute_script("var c = Components.classes;return 1;"))

