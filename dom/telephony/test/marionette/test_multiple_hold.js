


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const outNumber = "5555551111";
const outInfo = gOutCallStrPool(outNumber);
let outCall;

const inNumber = "5555552222";
const inInfo = gInCallStrPool(inNumber);
let inCall;

startTest(function() {
  gRemoteDial(inNumber)
    .then(call => inCall = call)
    .then(() => gCheckAll(null, [inCall], "", [], [inInfo.incoming]))

    
    .then(() => gAnswer(inCall))
    .then(() => gCheckAll(inCall, [inCall], "", [], [inInfo.active]))

    
    .then(() => gHold(inCall))
    .then(() => gCheckAll(null, [inCall], "", [], [inInfo.held]))

    
    .then(() => gDial(outNumber))
    .then(call => outCall = call)
    .then(() => gCheckAll(outCall, [inCall, outCall], "", [],
                          [inInfo.held, outInfo.ringing]))

    
    .then(() => gRemoteAnswer(outCall))
    .then(() => gCheckAll(outCall, [inCall, outCall], "", [],
                          [inInfo.held, outInfo.active]))

    
    
    .then(() => {
      let p1 = gWaitForNamedStateEvent(inCall, "connected");
      let p2 = gHold(outCall);
      return Promise.all([p1, p2]);
    })
    .then(() => gCheckAll(inCall, [inCall, outCall], "", [],
                          [inInfo.active, outInfo.held]))

    
    .then(() => {
      let p1 = gWaitForNamedStateEvent(outCall, "connected");
      let p2 = gHangUp(inCall);
      return Promise.all([p1, p2]);
    })
    .then(() => gCheckAll(outCall, [outCall], "", [], [outInfo.active]))
    .then(() => gHangUp(outCall))
    .then(() => gCheckAll(null, [], "", [], []))

    .catch(error => ok(false, "Promise reject: " + error))
    .then(finish);
});
