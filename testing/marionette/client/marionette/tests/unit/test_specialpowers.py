



from marionette_test import MarionetteTestCase
from errors import JavascriptException, MarionetteException

class TestSpecialPowers(MarionetteTestCase):

    def test_prefs(self):
        self.marionette.set_context("chrome")
        result = self.marionette.execute_script("""
        SpecialPowers.setCharPref("testing.marionette.charpref", "blabla");
        return SpecialPowers.getCharPref("testing.marionette.charpref")
        """);
        self.assertEqual(result, "blabla")
