


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const outNumber = "5555551111";
const outInfo = gOutCallStrPool(outNumber);
let outCall;

startTest(function() {
  gDial(outNumber)
    .then(call => outCall = call)
    .then(() => gCheckAll(outCall, [outCall], "", [], [outInfo.ringing]))

    
    .then(() => {
      let p1 = gWaitForStateChangeEvent(outCall, "connected");
      let p2 = gRemoteAnswer(outCall);
      return Promise.all([p1, p2]);
    })
    .then(() => gCheckAll(outCall, [outCall], "", [], [outInfo.active]))

    
    .then(() => {
      let p1 = gWaitForStateChangeEvent(outCall, "holding")
        .then(() => gWaitForStateChangeEvent(outCall, "held"));
      let p2 = gHold(outCall);
      return Promise.all([p1, p2]);
    })
    .then(() => gCheckAll(null, [outCall], "", [], [outInfo.held]))

    
    .then(() => {
      let p1 = gWaitForStateChangeEvent(outCall, "resuming")
        .then(() => gWaitForStateChangeEvent(outCall, "connected"));
      let p2 = gResume(outCall);
      return Promise.all([p1, p2]);
    })
    .then(() => gCheckAll(outCall, [outCall], "", [], [outInfo.active]))

    
    .then(() => {
      let p1 = gWaitForStateChangeEvent(outCall, "disconnecting")
        .then(() => gWaitForStateChangeEvent(outCall, "disconnected"));
      let p2 = gHangUp(outCall);
      return Promise.all([p1, p2]);
    })
    .then(() => gCheckAll(null, [], "", [], []))

    .catch(error => ok(false, "Promise reject: " + error))
    .then(finish);
});
