


'use strict';




const MARIONETTE_TIMEOUT = 60000;
const MARIONETTE_HEAD_JS = 'head.js';

const MANIFEST_URL = 'app://system.gaiamobile.org/manifest.webapp';
const NDEF_MESSAGE = [new MozNDEFRecord({tnf: "well-known",
                                         type: new Uint8Array(0x84),
                                         payload: new Uint8Array(0x20)})];

let nfcPeers = [];







function testNfcNotEnabledError() {
  log('testNfcNotEnabledError');
  toggleNFC(true)
  .then(() => activateAndwaitForTechDiscovered(emulator.P2P_RE_INDEX_0))
  .then(registerAndFireOnpeerready)
  .then(() => deactivateAndWaitForPeerLost())
  .then(() => toggleNFC(false))
  .then(() => sendNDEFExpectError(nfcPeers[0]))
  .then(endTest)
  .catch(handleRejectedPromise);
}








function testNfcBadSessionIdError() {
  log('testNfcBadSessionIdError');
  toggleNFC(true)
  .then(() => activateAndwaitForTechDiscovered(emulator.P2P_RE_INDEX_0))
  .then(registerAndFireOnpeerready)
  .then(() => NCI.deactivate())
  .then(() => activateAndwaitForTechDiscovered(emulator.P2P_RE_INDEX_0))
  .then(registerAndFireOnpeerready)
  
  .then(() => sendNDEFExpectError(nfcPeers[0]))
  .then(() => deactivateAndWaitForPeerLost())
  .then(() => toggleNFC(false))
  .then(endTest)
  .catch(handleRejectedPromise);
}






function testNoErrorInTechMsg() {
  log('testNoErrorInTechMsg');

  let techDiscoveredHandler = function(msg) {
    ok('Message handler for nfc-manager-tech-discovered');
    ok(msg.peer, 'check for correct tech type');
    is(msg.errorMsg, undefined, 'Should not get error msg in tech discovered');

    setAndFireTechLostHandler()
    .then(() => toggleNFC(false))
    .then(endTest)
    .catch(handleRejectedPromise);
  };

  sysMsgHelper.waitForTechDiscovered(techDiscoveredHandler);

  toggleNFC(true)
  .then(() => NCI.activateRE(emulator.P2P_RE_INDEX_0))
  .catch(handleRejectedPromise);
}

function endTest() {
  nfcPeers = [];
  runNextTest();
}

function handleRejectedPromise() {
  ok(false, 'Handling rejected promise');
  toggleNFC(false).then(endTest);
}

function registerAndFireOnpeerready() {
  let deferred = Promise.defer();

  nfc.onpeerready = function(event) {
    log("onpeerready called");
    nfcPeers.push(event.peer);
    nfc.onpeerready = null;
    deferred.resolve();
  };

  nfc.notifyUserAcceptedP2P(MANIFEST_URL);
  return deferred.promise;
}

function sendNDEFExpectError(peer) {
  let deferred = Promise.defer();

  peer.sendNDEF(NDEF_MESSAGE)
  .then(() => {
    deferred.reject();
  }).catch((e) => {
    ok(true, 'this should happen ' + e);
    deferred.resolve();
  });

  return deferred.promise;
}

function setAndFireTechLostHandler() {
  let deferred = Promise.defer();

  let techLostHandler = function(msg) {
    ok('Message handler for nfc-manager-tech-lost');
    is(msg.errorMsg, undefined, 'Should not get error msg in tech lost');

    deferred.resolve();
  };

  sysMsgHelper.waitForTechLost(techLostHandler);

  
  NCI.deactivate();
  return deferred.promise;
}

let tests = [
  testNfcNotEnabledError,


  testNoErrorInTechMsg
];







SpecialPowers.pushPermissions(
  [
    {'type': 'nfc-manager', 'allow': true, context: document},
    {'type': 'nfc-share', 'allow': true, context: document}
  ], runTests);
