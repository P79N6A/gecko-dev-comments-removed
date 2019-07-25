




import os
from marionette_test import MarionetteTestCase

class TestImportScript(MarionetteTestCase):
    def test_import_script(self):
        js = os.path.abspath(os.path.join(__file__, os.path.pardir, "importscript.js"))
        self.marionette.import_script(js)
        self.assertEqual("i'm a test function!", self.marionette.execute_script("return testFunc();"))
        self.assertEqual("i'm a test function!", self.marionette.execute_async_script("marionetteScriptFinished(testFunc());"))

class TestImportScriptChrome(TestImportScript):
    def setUp(self):
        MarionetteTestCase.setUp(self)
        self.marionette.set_context("chrome")
