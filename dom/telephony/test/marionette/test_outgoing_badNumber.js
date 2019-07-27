


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const number = "****5555552368****";
let outCall;

function testDialOutInvalidNumber() {
  log("Make an outCall call to an invalid number.");

  
  
  
  return telephony.dial(number).then(call => {
    outCall = call;
    ok(outCall);
    is(outCall.id.number, "");  
    is(outCall.state, "dialing");

    is(outCall, telephony.active);
    is(telephony.calls.length, 1);
    is(telephony.calls[0], outCall);

    return gWaitForEvent(outCall, "error").then(event => {
      is(event.call, outCall);
      ok(event.call.error);
      is(event.call.error.name, "BadNumberError");
      is(event.call.disconnectedReason, "BadNumber");
    })
    .then(() => gCheckAll(null, [], "", [], []));
  });
}

startTest(function() {
  testDialOutInvalidNumber()
    .catch(error => ok(false, "Promise reject: " + error))
    .then(finish);
});
