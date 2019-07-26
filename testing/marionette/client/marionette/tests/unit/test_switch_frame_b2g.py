



from marionette_test import MarionetteTestCase
from marionette import *

class TestGaiaLaunch(MarionetteTestCase):
    """Trivial example of launching a Gaia app, entering its context and performing some test on it.
    """

    def test_launch_app(self):
        
        
        
        app_frame = self.launch_gaia_app('../sms/sms.html')

        
        
        page_title = self.marionette.execute_script("""
var frame = arguments[0];
return frame.contentWindow.document.getElementsByTagName('title')[0].innerHTML;
""", [app_frame])
        self.assertEqual(page_title, 'Messages')

        self.marionette.switch_to_frame(0)
        self.assertEqual(self.marionette.execute_script("return window.document.getElementsByTagName('title')[0].innerHTML;"), 'Messages')
        self.assertTrue("sms" in self.marionette.execute_script("return document.location.href;"))
        self.marionette.switch_to_frame()
        self.assertTrue("homescreen" in self.marionette.execute_script("return document.location.href;"))

