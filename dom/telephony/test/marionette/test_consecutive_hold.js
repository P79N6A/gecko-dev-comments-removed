


MARIONETTE_TIMEOUT = 90000;
MARIONETTE_HEAD_JS = 'head.js';

const inNumber = "5555552222";
const inInfo = gInCallStrPool(inNumber);
let inCall;

startTest(function() {
  Promise.resolve()

    
    .then(() => gRemoteDial(inNumber))
    .then(call => inCall = call)
    .then(() => gCheckAll(null, [inCall], "", [], [inInfo.incoming]))
    .then(() => is(inCall.disconnectedReason, null))

    
    .then(() => gAnswer(inCall))
    .then(() => gCheckAll(inCall, [inCall], "", [], [inInfo.active]))
    .then(() => is(inCall.disconnectedReason, null))

    
    
    .then(() => emulator.runCmd("gsm disable hold"))
    .then(() => gHold(inCall))
    .then(() => ok(false, "This hold request should be rejected."),
          () => log("This hold request is rejected as expected."))
    .then(() => gCheckAll(inCall, [inCall], "", [], [inInfo.active]))
    .then(() => is(inCall.disconnectedReason, null))

    
    
    .then(() => emulator.runCmd("gsm enable hold"))
    .then(() => gHold(inCall))
    .then(() => log("This hold request is resolved as expected."),
          () => ok(false, "This hold request should be resolved."))
    .then(() => gCheckAll(null, [inCall], "", [], [inInfo.held]))
    .then(() => is(inCall.disconnectedReason, null))

    
    .then(() => gHangUp(inCall))
    .then(() => gCheckAll(null, [], "", [], []))
    .then(() => is(inCall.disconnectedReason, "NormalCallClearing"))

    
    .catch(error => ok(false, "Promise reject: " + error))
    .then(() => emulator.runCmd("gsm enable hold"))
    .then(finish);
});
