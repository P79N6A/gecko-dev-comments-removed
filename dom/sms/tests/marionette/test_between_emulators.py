from marionette_test import *


class SMSTest(MarionetteTestCase):

    @unittest.expectedFailure
    def test_sms_between_emulators(self):
        
        
        
        sender = self.get_new_emulator()
        receiver = self.marionette

        self.set_up_test_page(sender, "test.html", ["sms"])
        self.set_up_test_page(receiver, "test.html", ["sms"])

        
        
        message = 'hello world!'
        self.assertTrue(receiver.execute_script("return window.navigator.mozSms != null;"))
        receiver.execute_script("""
global.smsreceived = null;
window.navigator.mozSms.addEventListener("received", function(e) {
    global.smsreceived = e.message.body;
});
""", new_sandbox=False)

        
        sender.execute_script("""
window.navigator.mozSms.send("%d", "%s");
""" % (receiver.emulator.port, message))

        
        
        receiver.set_script_timeout(0) 
        received = receiver.execute_async_script("""
        waitFor(function () {
            marionetteScriptFinished(global.smsreceived);
        }, function () {
            return global.smsreceived
        });
        """, new_sandbox=False)
        self.assertEqual(received, message)
