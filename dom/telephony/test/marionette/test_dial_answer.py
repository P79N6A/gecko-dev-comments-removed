from marionette_test import *


class MultiEmulatorDialTest(MarionetteTestCase):
    """A simple test which verifies the ability of one emulator to dial
       another and to detect an incoming call.
    """

    def test_dial_answer(self):
        
        
        
        sender = self.get_new_emulator()
        receiver = self.marionette

        self.set_up_test_page(sender, "test.html", ["dom.telephony.app.phone.url"])
        self.set_up_test_page(receiver, "test.html", ["dom.telephony.app.phone.url"])

        
        
        self.assertTrue(receiver.execute_script("""
return window.navigator.mozTelephony != null
"""))
        receiver.execute_script("""
global.incoming = null;
window.navigator.mozTelephony.addEventListener("incoming", function test_incoming(e) {
    window.navigator.mozTelephony.removeEventListener("incoming", test_incoming);
    global.incoming = e.call;
});
""", new_sandbox=False)

        
        toPhoneNumber = "1555521%d" % receiver.emulator.port
        fromPhoneNumber = "1555521%d" % sender.emulator.port
        sender.execute_script("""
global.call = window.navigator.mozTelephony.dial("%s");
""" % toPhoneNumber, new_sandbox=False)

        
        
        
        receiver.set_script_timeout(30000)
        received = receiver.execute_async_script("""
global.callstate = null;
waitFor(function() {
    let call = global.incoming;
    call.addEventListener("connected", function test_connected(e) {
        call.removeEventListener("connected", test_connected);
        global.callstate = e.call.state;
    });
    marionetteScriptFinished(call.number);
},
function() {
    return global.incoming != null;
});
""", new_sandbox=False)
        
        self.assertEqual(received, fromPhoneNumber)

        
        
        sender.execute_script("""
let call = global.call;
global.callstate = null;
call.addEventListener("connected", function test_connected(e) {
    call.removeEventListener("connected", test_connected);
    global.callstate = e.call.state;
});
""", new_sandbox=False)

        
        
        receiver.execute_async_script("""
global.incoming.answer();
waitFor(function() {
    marionetteScriptFinished(true);
}, function() {
    return global.callstate == "connected";
});
""", new_sandbox=False)

        
        self.assertTrue(receiver.execute_async_script("""
waitFor(function() {
    global.incoming.hangUp();
    marionetteScriptFinished(true);
 }, function() {
    return global.callstate == "connected";
});
""", new_sandbox=False))
