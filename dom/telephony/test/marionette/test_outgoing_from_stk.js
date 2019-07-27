


MARIONETTE_TIMEOUT = 90000;
MARIONETTE_HEAD_JS = 'head.js';

const outNumber = "5555552222";
const outInfo = gOutCallStrPool(outNumber);
let outCall;



function outgoing() {
  return gDialSTK(outNumber)
    .then(call => outCall = call)
    .then(() => gCheckAll(outCall, [outCall], "", [], [outInfo.ringing]));
}

function localHangUp() {
  return gHangUp(outCall)
    .then(() => gCheckAll(null, [], "", [], []));
}

function remoteAnswer() {
  return gRemoteAnswer(outCall)
    .then(() => gCheckAll(outCall, [outCall], "", [], [outInfo.active]));
}

function remoteHangUp() {
  return gRemoteHangUp(outCall)
    .then(() => gCheckAll(null, [], "", [], []));
}



function testOutgoingLocalHangUp(){
  log("= testOutgoingLocalHangUp =");
  return outgoing()
    .then(() => localHangUp());
}

function testOutgoingRemoteHangUp() {
  log("= testOutgoingRemoteHangUp =");
  return outgoing()
    .then(() => remoteHangUp());
}

function testOutgoingRemoteAnswerRemoteHangUp() {
  log("= testOutgoingRemoreAnswerRemoteHangUp =");
  return outgoing()
    .then(() => remoteAnswer())
    .then(() => remoteHangUp());
}



startTest(function() {
  Promise.resolve()
    .then(() => testOutgoingLocalHangUp())
    .then(() => testOutgoingRemoteHangUp())
    .then(() => testOutgoingRemoteAnswerRemoteHangUp())

    .catch(error => ok(false, "Promise reject: " + error))
    .then(finish);
});
