



from marionette import MarionetteTestCase
from marionette_driver.errors import JavascriptException


class TestExecuteSandboxes(MarionetteTestCase):
    def setUp(self):
        super(TestExecuteSandboxes, self).setUp()

    def test_execute_system_sandbox(self):
        
        result = self.marionette.execute_script("""
            return Components.interfaces.nsIPermissionManager.ALLOW_ACTION;
            """, sandbox='system')
        self.assertEqual(result, 1)

    def test_execute_async_system_sandbox(self):
        
        
        result = self.marionette.execute_async_script("""
            let result = Components.interfaces.nsIPermissionManager.ALLOW_ACTION;
            marionetteScriptFinished(result);
            """, sandbox='system')
        self.assertEqual(result, 1)

    def test_execute_switch_sandboxes(self):
        
        
        self.marionette.execute_script("foo = 1;", sandbox='1')
        self.marionette.execute_script("foo = 2;", sandbox='2')
        foo = self.marionette.execute_script("return foo;", sandbox='1',
                                             new_sandbox=False)
        self.assertEqual(foo, 1)
        foo = self.marionette.execute_script("return foo;", sandbox='2',
                                             new_sandbox=False)
        self.assertEqual(foo, 2)

    def test_execute_new_sandbox(self):
        
        self.marionette.execute_script("foo = 1;", sandbox='1')
        self.marionette.execute_script("foo = 2;", sandbox='2')
        self.assertRaises(JavascriptException,
                          self.marionette.execute_script,
                          "return foo;", sandbox='1', new_sandbox=True)
        foo = self.marionette.execute_script("return foo;", sandbox='2',
                                             new_sandbox=False)
        self.assertEqual(foo, 2)

    def test_execute_async_switch_sandboxes(self):
        
        
        self.marionette.execute_async_script("foo = 1; marionetteScriptFinished()",
                                             sandbox='1')
        self.marionette.execute_async_script("foo = 2; marionetteScriptFinished()",
                                             sandbox='2')
        foo = self.marionette.execute_async_script("marionetteScriptFinished(foo);",
                                                   sandbox='1',
                                                   new_sandbox=False)
        self.assertEqual(foo, 1)
        foo = self.marionette.execute_async_script("marionetteScriptFinished(foo);",
                                                   sandbox='2',
                                                   new_sandbox=False)
        self.assertEqual(foo, 2)


class TestExecuteSandboxesChrome(TestExecuteSandboxes):
    def setUp(self):
        super(TestExecuteSandboxesChrome, self).setUp()
        self.marionette.set_context("chrome")
