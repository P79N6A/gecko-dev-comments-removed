from marionette_test import *


class MultiEmulatorDialTest(MarionetteTestCase):
    """A simple test which verifies the ability of one emulator to dial
       another and to detect an incoming call.
    """

    def test_dial_between_emulators(self):
        
        
        
        sender = self.get_new_emulator()
        receiver = self.marionette

        self.set_up_test_page(sender, "test.html", ["dom.telephony.app.phone.url"])
        self.set_up_test_page(receiver, "test.html", ["dom.telephony.app.phone.url"])

        
        
        self.assertTrue(receiver.execute_script("""
return window.navigator.mozTelephony != undefined && window.navigator.mozTelephony != null;
""", new_sandbox=False))
        receiver.execute_script("""
global.incoming = null;
window.navigator.mozTelephony.addEventListener("incoming", function(e) {
    global.incoming = e.call.number;
});
""", new_sandbox=False)

        
        toPhoneNumber = "1555521%d" % receiver.emulator.port
        fromPhoneNumber = "1555521%d" % sender.emulator.port
        sender.execute_script("""
window.navigator.mozTelephony.dial("%s");
""" % toPhoneNumber, new_sandbox=False)

        
        
        
        receiver.set_script_timeout(10000)
        received = receiver.execute_async_script("""
        waitFor(function () {
            marionetteScriptFinished(global.incoming);
        }, function () {
            return global.incoming;
        });
        """, new_sandbox=False)
        
        self.assertEqual(received, fromPhoneNumber)

        sender.execute_script("window.navigator.mozTelephony.calls[0].hangUp();", new_sandbox=False)
        receiver.execute_script("window.navigator.mozTelephony.calls[0].hangUp();", new_sandbox=False)
