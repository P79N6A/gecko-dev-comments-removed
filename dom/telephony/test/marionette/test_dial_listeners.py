from marionette_test import *


class DialListenerTest(MarionetteTestCase):
    """A test of some of the different listeners for nsIDOMTelephonyCall.
    """

    def test_dial_listeners(self):
        
        
        
        sender = self.get_new_emulator()
        receiver = self.marionette

        receiver.set_script_timeout(30000)
        sender.set_script_timeout(30000)
        receiver.set_context("chrome")
        sender.set_context("chrome")
        toPhoneNumber = "1555521%d" % receiver.emulator.port
        fromPhoneNumber = "1555521%d" % sender.emulator.port

        
        
        self.assertTrue(receiver.execute_script("""
return window.navigator.mozTelephony != undefined && window.navigator.mozTelephony != null;
"""))
        receiver.execute_script("""
window.wrappedJSObject.incoming = null;
window.navigator.mozTelephony.addEventListener("incoming", function test_incoming(e) {
    window.navigator.mozTelephony.removeEventListener("incoming", test_incoming);
    window.wrappedJSObject.incoming = e.call;
});
""")

        
        sender.execute_script("""
window.wrappedJSObject.sender_state = [];
window.wrappedJSObject.sender_call = window.navigator.mozTelephony.dial("%s");
window.wrappedJSObject.sender_call.addEventListener("statechange", function test_sender_statechange(e) {
    if (e.call.state == 'disconnected')
        window.wrappedJSObject.sender_call.removeEventListener("statechange", test_sender_statechange);
    window.wrappedJSObject.sender_state.push(e.call.state);
});
window.wrappedJSObject.sender_alerting = false;
window.wrappedJSObject.sender_call.addEventListener("alerting", function test_sender_alerting(e) {
    window.wrappedJSObject.sender_call.removeEventListener("alerting", test_sender_alerting);
    window.wrappedJSObject.sender_alerting = e.call.state == 'alerting';
});
""" % toPhoneNumber)

        
        
        
        received = receiver.execute_async_script("""
window.wrappedJSObject.receiver_state = [];
waitFor(function() {
    let call = window.wrappedJSObject.incoming;
    call.addEventListener("statechange", function test_statechange(e) {
        if (e.call.state == 'disconnected')
            call.removeEventListener("statechange", test_statechange);
        window.wrappedJSObject.receiver_state.push(e.call.state);
    });
    call.addEventListener("connected", function test_connected(e) {
        call.removeEventListener("connected", test_connected);
        window.wrappedJSObject.receiver_connected = e.call.state == 'connected';
    });
    marionetteScriptFinished(call.number);
},
function() {
    return window.wrappedJSObject.incoming != null;
});
""")
        
        self.assertEqual(received, fromPhoneNumber)

        
        
        self.assertTrue('alerting' in sender.execute_script("return window.wrappedJSObject.sender_state;"))
        self.assertTrue(sender.execute_script("return window.wrappedJSObject.sender_alerting;"))

        
        
        receiver.execute_async_script("""
window.wrappedJSObject.incoming.answer();
waitFor(function() {
    marionetteScriptFinished(true);
}, function() {
    return window.wrappedJSObject.receiver_connected;
});
""")
        state = receiver.execute_script("return window.wrappedJSObject.receiver_state;")
        self.assertTrue('connecting' in state)
        self.assertTrue('connected' in state)

        
        self.assertTrue('connected' in sender.execute_async_script("""
waitFor(function() {
    marionetteScriptFinished(window.wrappedJSObject.sender_state);
}, function() {
    return window.wrappedJSObject.sender_call.state == "connected";
});
"""))

        
        sender.execute_script("""
window.wrappedJSObject.sender_disconnected = null;
window.wrappedJSObject.sender_call.addEventListener("disconnected", function test_disconnected(e) {
    window.wrappedJSObject.sender_call.removeEventListener("disconnected", test_disconnected);
    window.wrappedJSObject.sender_disconnected = e.call.state == 'disconnected';
});
""")
        receiver.execute_script("""
window.wrappedJSObject.receiver_disconnected = null;
window.wrappedJSObject.incoming.addEventListener("disconnected", function test_disconnected(e) {
    window.wrappedJSObject.incoming.removeEventListener("disconnected", test_disconnected);
    window.wrappedJSObject.receiver_disconnected = e.call.state == 'disconnected';
});
""")

        
        receiver.execute_script("""
window.wrappedJSObject.incoming.hangUp();
""")

        
        
        sender_state = sender.execute_async_script("""
waitFor(function() {
    marionetteScriptFinished(window.wrappedJSObject.sender_state);
}, function () {
    return window.wrappedJSObject.sender_call.state == 'disconnected';
});
""")
        self.assertTrue('disconnected' in sender_state)
        self.assertTrue(sender.execute_script("return window.wrappedJSObject.sender_disconnected;"))

        
        
        state = receiver.execute_async_script("""
waitFor(function() {
    marionetteScriptFinished(window.wrappedJSObject.receiver_state);
}, function () {
    return window.wrappedJSObject.incoming.state == 'disconnected';
});
""")
        self.assertTrue('disconnected' in state)
        self.assertTrue('disconnecting' in state)
        self.assertTrue(receiver.execute_script("return window.wrappedJSObject.receiver_disconnected;"))

