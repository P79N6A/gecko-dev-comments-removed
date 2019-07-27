


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

let number = "****5555552368****";
let outgoing;


function dial() {
  log("Make an outgoing call to an invalid number.");

  
  
  
  telephony.dial(number).then(call => {
    outgoing = call;
    ok(outgoing);
    is(outgoing.id.number, number);
    is(outgoing.state, "dialing");

    is(outgoing, telephony.active);
    is(telephony.calls.length, 1);
    is(telephony.calls[0], outgoing);

    outgoing.onerror = function onerror(event) {
      log("Received 'error' event.");
      is(event.call, outgoing);
      ok(event.call.error);
      is(event.call.error.name, "BadNumberError");

      emulator.runCmdWithCallback("gsm list", function(result) {
        log("Initial call list: " + result);
        cleanUp();
      });
    };
  });
}

function cleanUp() {
  finish();
}

startTest(function() {
  dial();
});
