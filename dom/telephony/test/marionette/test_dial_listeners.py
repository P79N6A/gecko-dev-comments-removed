from marionette_test import *


class DialListenerTest(MarionetteTestCase):
    """A test of some of the different listeners for nsIDOMTelephonyCall.
    """

    def test_dial_listeners(self):
        
        
        
        sender = self.get_new_emulator()
        receiver = self.marionette

        self.set_up_test_page(sender, "test.html", ["dom.telephony.app.phone.url"])
        self.set_up_test_page(receiver, "test.html", ["dom.telephony.app.phone.url"])

        receiver.set_script_timeout(10000)
        sender.set_script_timeout(10000)

        toPhoneNumber = "1555521%d" % receiver.emulator.port
        fromPhoneNumber = "1555521%d" % sender.emulator.port

        
        
        self.assertTrue(receiver.execute_script("""
return window.navigator.mozTelephony != undefined && window.navigator.mozTelephony != null;
""", new_sandbox=False))
        receiver.execute_script("""
global.incoming = null;
window.navigator.mozTelephony.addEventListener("incoming", function test_incoming(e) {
    window.navigator.mozTelephony.removeEventListener("incoming", test_incoming);
    global.incoming = e.call;
});
""", new_sandbox=False)

        
        sender.execute_script("""
global.sender_state = [];
global.sender_call = window.navigator.mozTelephony.dial("%s");
global.sender_call.addEventListener("statechange", function test_sender_statechange(e) {
    if (e.call.state == 'disconnected')
        global.sender_call.removeEventListener("statechange", test_sender_statechange);
    global.sender_state.push(e.call.state);
});
global.sender_alerting = false;
global.sender_call.addEventListener("alerting", function test_sender_alerting(e) {
    global.sender_call.removeEventListener("alerting", test_sender_alerting);
    global.sender_alerting = e.call.state == 'alerting';
});
""" % toPhoneNumber, new_sandbox=False)

        
        
        
        received = receiver.execute_async_script("""
global.receiver_state = [];
waitFor(function() {
    let call = global.incoming;
    call.addEventListener("statechange", function test_statechange(e) {
        if (e.call.state == 'disconnected')
            call.removeEventListener("statechange", test_statechange);
        global.receiver_state.push(e.call.state);
    });
    call.addEventListener("connected", function test_connected(e) {
        call.removeEventListener("connected", test_connected);
        global.receiver_connected = e.call.state == 'connected';
    });
    marionetteScriptFinished(call.number);
},
function() {
    return global.incoming != null;
});
""", new_sandbox=False)
        
        self.assertEqual(received, fromPhoneNumber)

        
        
        self.assertTrue('alerting' in sender.execute_script("return global.sender_state;", new_sandbox=False))
        self.assertTrue(sender.execute_script("return global.sender_alerting;", new_sandbox=False))

        
        
        receiver.execute_async_script("""
global.incoming.answer();
waitFor(function() {
    marionetteScriptFinished(true);
}, function() {
    return global.receiver_connected;
});
""", new_sandbox=False)
        state = receiver.execute_script("return global.receiver_state;", new_sandbox=False)
        self.assertTrue('connecting' in state)
        self.assertTrue('connected' in state)

        
        self.assertTrue('connected' in sender.execute_async_script("""
waitFor(function() {
    marionetteScriptFinished(global.sender_state);
}, function() {
    return global.sender_call.state == "connected";
});
""", new_sandbox=False))

        
        sender.execute_script("""
global.sender_disconnected = null;
global.sender_call.addEventListener("disconnected", function test_disconnected(e) {
    global.sender_call.removeEventListener("disconnected", test_disconnected);
    global.sender_disconnected = e.call.state == 'disconnected';
});
""", new_sandbox=False)
        receiver.execute_script("""
global.receiver_disconnected = null;
global.incoming.addEventListener("disconnected", function test_disconnected(e) {
    global.incoming.removeEventListener("disconnected", test_disconnected);
    global.receiver_disconnected = e.call.state == 'disconnected';
});
""", new_sandbox=False)

        
        receiver.execute_script("""
global.incoming.hangUp();
""", new_sandbox=False)

        
        
        sender_state = sender.execute_async_script("""
waitFor(function() {
    marionetteScriptFinished(global.sender_state);
}, function () {
    return global.sender_call.state == 'disconnected';
});
""", new_sandbox=False)
        self.assertTrue('disconnected' in sender_state)
        self.assertTrue(sender.execute_script("return global.sender_disconnected;", new_sandbox=False))

        
        
        state = receiver.execute_async_script("""
waitFor(function() {
    marionetteScriptFinished(global.receiver_state);
}, function () {
    return global.incoming.state == 'disconnected';
});
""", new_sandbox=False)
        self.assertTrue('disconnected' in state)
        self.assertTrue('disconnecting' in state)
        self.assertTrue(receiver.execute_script("return global.receiver_disconnected;", new_sandbox=False))
