


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const inNumber = "5555552222";
const inInfo = gInCallStrPool(inNumber);
let inCall;

function incoming() {
  return gRemoteDial(inNumber)
    .then(call => inCall = call)
    .then(() => gCheckAll(null, [inCall], "", [], [inInfo.incoming]));
}

function connecting() {
  let promises = [
    gWaitForNamedStateEvent(inCall, "connecting"),
    inCall.answer()
  ];
  return Promise.all(promises).then(() => inCall);
}

function hangUp() {
  return gHangUp(inCall)
    .then(() => gCheckAll(null, [], "", [], []));
}

function remoteHangUp() {
  return gRemoteHangUp(inCall)
    .then(() => gCheckAll(null, [], "", [], []));
}



function testConnectingHangUp() {
  log("= testConnectingHangUp =");
  return incoming()
    .then(() => connecting())
    .then(() => hangUp());
}

function testConnectingRemoteHangUp() {
  log("= testConnectingRemoteHangUp =");
  return incoming()
    .then(() => connecting())
    .then(() => remoteHangUp());
}

startTest(function() {
  Promise.resolve()
    .then(() => testConnectingHangUp())
    .then(() => testConnectingRemoteHangUp())

    .catch(error => ok(false, "Promise reject: " + error))
    .then(finish);
});
