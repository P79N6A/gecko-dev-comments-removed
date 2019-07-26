


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

function testConferenceTwoCalls() {
  log('= testConferenceTwoCalls =');

  let outCall;
  let inCall;
  let outNumber = "5555550101";
  let inNumber  = "5555550201";

  return Promise.resolve()
    .then(() => gSetupConference([outNumber, inNumber]))
    .then(calls => {
      [outCall, inCall] = calls;
    })
    .then(() => gRemoteHangUpCalls([outCall, inCall]));
}


startTest(function() {
  testConferenceTwoCalls()
    .then(null, error => {
      ok(false, 'promise rejects during test.');
    })
    .then(finish);
});
