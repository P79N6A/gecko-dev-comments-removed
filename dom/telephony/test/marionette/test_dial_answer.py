from marionette_test import *


class MultiEmulatorDialTest(MarionetteTestCase):
    """A simple test which verifies the ability of one emulator to dial
       another and to detect an incoming call.
    """

    def test_dial_answer(self):
        
        
        
        sender = self.get_new_emulator()
        receiver = self.marionette

        
        
        receiver.set_context("chrome")
        self.assertTrue(receiver.execute_script("""
return navigator.mozTelephony != undefined && navigator.mozTelephony != null;
"""))
        receiver.execute_script("""
window.wrappedJSObject.incoming = null;
navigator.mozTelephony.addEventListener("incoming", function test_incoming(e) {
    navigator.mozTelephony.removeEventListener("incoming", test_incoming);
    window.wrappedJSObject.incoming = e.call;
});
""")

        
        toPhoneNumber = "1555521%d" % receiver.emulator.port
        fromPhoneNumber = "1555521%d" % sender.emulator.port
        sender.set_context("chrome")
        sender.execute_script("""
window.wrappedJSObject.call = navigator.mozTelephony.dial("%s");
""" % toPhoneNumber)

        
        
        
        receiver.set_script_timeout(30000)
        received = receiver.execute_async_script("""
window.wrappedJSObject.callstate = null;
waitFor(function() {
    let call = window.wrappedJSObject.incoming;
    call.addEventListener("connected", function test_connected(e) {
        call.removeEventListener("connected", test_connected);
        window.wrappedJSObject.callstate = e.call.state;
    });
    marionetteScriptFinished(call.number);
},
function() {
    return window.wrappedJSObject.incoming != null;
});
""")
        
        self.assertEqual(received, fromPhoneNumber)

        
        
        sender.execute_script("""
let call = window.wrappedJSObject.call;
window.wrappedJSObject.callstate = null;
call.addEventListener("connected", function test_connected(e) {
    call.removeEventListener("connected", test_connected);
    window.wrappedJSObject.callstate = e.call.state;
});
""")

        
        
        receiver.execute_async_script("""
window.wrappedJSObject.incoming.answer();
waitFor(function() {
    marionetteScriptFinished(true);
}, function() {
    return window.wrappedJSObject.callstate == "connected";
});
""")

        
        self.assertTrue(receiver.execute_async_script("""
waitFor(function() {
    marionetteScriptFinished(true);
}, function() {
    return window.wrappedJSObject.callstate == "connected";
});
"""))

