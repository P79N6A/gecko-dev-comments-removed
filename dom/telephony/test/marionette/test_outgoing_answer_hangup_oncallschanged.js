


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const outNumber = "5555551111";
const outInfo = gOutCallStrPool(outNumber);
let outCall;

startTest(function() {
  Promise.resolve()
    
    .then(() => {
      let tmpCall;

      let p1 = gWaitForCallsChangedEvent(telephony)
        .then(call => tmpCall = call);

      let p2 = gDial(outNumber)
        .then(call => outCall = call);

      return Promise.all([p1, p2]).then(() => is(outCall, tmpCall));
    })
    .then(() => gCheckAll(outCall, [outCall], "", [], [outInfo.ringing]))

    .then(() => gRemoteAnswer(outCall))
    .then(() => gCheckAll(outCall, [outCall], "", [], [outInfo.active]))

    
    .then(() => {
      let p1 = gWaitForCallsChangedEvent(telephony, outCall);
      let p2 = gRemoteHangUp(outCall);
      return Promise.all([p1, p2]);
    })
    .then(() => gCheckAll(null, [], "", [], []))

    .catch(error => ok(false, "Promise reject: " + error))
    .then(finish);
});
