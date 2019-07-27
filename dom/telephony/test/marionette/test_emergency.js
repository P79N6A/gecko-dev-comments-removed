


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

startTest(function() {
  let outCall;

  gDialEmergency("911")
    .then(call => outCall = call)
    .then(() => gRemoteAnswer(outCall))
    .then(() => gHangUp(outCall))
    .catch(error => ok(false, "Promise reject: " + error))
    .then(finish);
});
