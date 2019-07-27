


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const AUDIO_MANAGER_CONTRACT_ID = "@mozilla.org/telephony/audiomanager;1";


const PHONE_STATE_INVALID          = -2;
const PHONE_STATE_CURRENT          = -1;
const PHONE_STATE_NORMAL           = 0;
const PHONE_STATE_RINGTONE         = 1;
const PHONE_STATE_IN_CALL          = 2;
const PHONE_STATE_IN_COMMUNICATION = 3;

let audioManager = SpecialPowers.Cc[AUDIO_MANAGER_CONTRACT_ID]
                                .getService(SpecialPowers.Ci.nsIAudioManager);

ok(audioManager, "nsIAudioManager instance");

function check(phoneState) {
  return new Promise(function(resolve, reject) {
    waitFor(function() {
      resolve();
    }, function() {
      let currentPhoneState = audioManager.phoneState;
      log("waiting.. audioState should change to " + phoneState +
          ", current is" + currentPhoneState);

      return (phoneState == currentPhoneState ||
              (phoneState == PHONE_STATE_CURRENT &&
               currentPhoneState == PHONE_STATE_NORMAL));
    });
  });
}


startTest(function() {
  let outNumber = "5555550101";
  let inNumber  = "5555550201";
  let outCall;
  let inCall;

  Promise.resolve()
    .then(() => check(PHONE_STATE_CURRENT))

    
    .then(() => gRemoteDial(inNumber))
    .then(call => { inCall = call; })
    .then(() => check(PHONE_STATE_RINGTONE))
    .then(() => gAnswer(inCall))
    .then(() => check(PHONE_STATE_IN_CALL))
    
    .then(() => gRemoteHangUp(inCall))
    .then(() => check(PHONE_STATE_NORMAL))

    
    .then(() => gDial(outNumber))
    .then(call => { outCall = call; })
    .then(() => check(PHONE_STATE_IN_CALL))
    .then(() => gRemoteAnswer(outCall))
    .then(() => check(PHONE_STATE_IN_CALL))
    
    .then(() => gRemoteHangUp(outCall))
    .then(() => check(PHONE_STATE_NORMAL))

    
    .then(() => gDial(outNumber))
    .then(call => { outCall = call; })
    .then(() => check(PHONE_STATE_IN_CALL))
    .then(() => gRemoteAnswer(outCall))
    .then(() => check(PHONE_STATE_IN_CALL))
    .then(() => gHold(outCall))
    .then(() => check(PHONE_STATE_IN_CALL))
    .then(() => gResume(outCall))
    .then(() => check(PHONE_STATE_IN_CALL))
    
    .then(() => gRemoteDial(inNumber))
    .then(call => { inCall = call; })
    .then(() => check(PHONE_STATE_IN_CALL))
    .then(() => gAnswer(inCall))
    .then(() => check(PHONE_STATE_IN_CALL))
    
    .then(() => gAddCallsToConference([outCall, inCall]))
    .then(() => check(PHONE_STATE_IN_CALL))
    
    .then(() => gRemoteHangUpCalls([outCall, inCall]))
    .then(() => check(PHONE_STATE_NORMAL))

    
    .then(null, error => {
      ok(false, 'promise rejects during test.');
    })
    .then(finish);
});
