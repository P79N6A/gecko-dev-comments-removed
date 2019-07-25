from marionette_test import *


class MultiEmulatorDialTest(MarionetteTestCase):
    """A simple test which verifies the ability of one emulator to dial
       another and to detect an incoming call.
    """

    def test_dial_between_emulators(self):
        
        
        
        sender = self.get_new_emulator()
        receiver = self.marionette

        
        
        receiver.set_context("chrome")
        self.assertTrue(receiver.execute_script("""
return navigator.mozTelephony != undefined && navigator.mozTelephony != null;
"""))
        receiver.execute_script("""
window.wrappedJSObject.incoming = "none";
navigator.mozTelephony.addEventListener("incoming", function(e) {
    window.wrappedJSObject.incoming = e.call.number;
});
""")

        
        toPhoneNumber = "1555521%d" % receiver.emulator.port
        fromPhoneNumber = "1555521%d" % sender.emulator.port
        sender.set_context("chrome")
        sender.execute_script("""
navigator.mozTelephony.dial("%s");
""" % toPhoneNumber)

        
        
        
        receiver.set_script_timeout(30000)
        received = receiver.execute_async_script("""
        function check_incoming() {
            if (window.wrappedJSObject.incoming != "none") {
                marionetteScriptFinished(window.wrappedJSObject.incoming);
            }
            else {
                setTimeout(check_incoming, 500);
            }
        }
        setTimeout(check_incoming, 0);
    """)
        
        self.assertEqual(received, fromPhoneNumber)


