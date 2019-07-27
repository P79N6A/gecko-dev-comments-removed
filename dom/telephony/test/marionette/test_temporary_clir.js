


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const clirPrefix = "*31#";
const number = "0912345678";

startTest(function() {
  let outCall;

  telephony.dial(clirPrefix + number).then(call => {
    outCall = call;

    ok(call);
    is(call.id.number, number);  
  })
  .then(() => gRemoteHangUp(outCall))
  .catch(error => ok(false, "Promise reject: " + error))
  .then(finish);
});
