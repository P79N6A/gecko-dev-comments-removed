


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

function testConferenceHoldAndResume() {
  log('= testConferenceHoldAndResume =');

  let outCall;
  let inCall;
  let outNumber = "5555550101";
  let inNumber  = "5555550201";
  let outInfo = gOutCallStrPool(outNumber);
  let inInfo = gInCallStrPool(inNumber);

  return Promise.resolve()
    .then(() => gSetupConferenceTwoCalls(outNumber, inNumber))
    .then(calls => {
      [outCall, inCall] = calls;
    })
    .then(() => gHoldConference([outCall, inCall], function() {
      gCheckState(null, [], 'held', [outCall, inCall]);
    }))
    .then(() => gCheckAll(null, [], 'held', [outCall, inCall],
                          [outInfo.held, inInfo.held]))
    .then(() => gResumeConference([outCall, inCall], function() {
      gCheckState(conference, [], 'connected', [outCall, inCall]);
    }))
    .then(() => gCheckAll(conference, [], 'connected', [outCall, inCall],
                          [outInfo.active, inInfo.active]))
    .then(() => gRemoteHangUpCalls([outCall, inCall]));
}


startTest(function() {
  testConferenceHoldAndResume()
    .then(null, error => {
      ok(false, 'promise rejects during test.');
    })
    .then(finish);
});
