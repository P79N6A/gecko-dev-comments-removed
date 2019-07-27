


'use strict';




const MARIONETTE_TIMEOUT = 60000;
const MARIONETTE_HEAD_JS = 'head.js';

const MANIFEST_URL = 'app://system.gaiamobile.org/manifest.webapp';
const NDEF_MESSAGE = [new MozNDEFRecord(0x01,
                                        new Uint8Array(0x84),
                                        new Uint8Array(0),
                                        new Uint8Array(0x20))];

let nfcPeers = [];
let sessionTokens = [];







function testNfcNotEnabledError() {
  log('testNfcNotEnabledError');
  toggleNFC(true)
  .then(() => NCI.activateRE(emulator.P2P_RE_INDEX_0))
  .then(registerAndFireOnpeerready)
  .then(() => toggleNFC(false))
  .then(() => sendNDEFExpectError(nfcPeers[0], 'NfcNotEnabledError'))
  .then(endTest)
  .catch(handleRejectedPromise);
}








function testNfcBadSessionIdError() {
  log('testNfcBadSessionIdError');
  toggleNFC(true)
  .then(() => NCI.activateRE(emulator.P2P_RE_INDEX_0))
  .then(registerAndFireOnpeerready)
  .then(() => NCI.deactivate())
  .then(() => NCI.activateRE(emulator.P2P_RE_INDEX_0))
  .then(registerAndFireOnpeerready)
  
  .then(() => sendNDEFExpectError(nfcPeers[0], 'NfcBadSessionIdError'))
  .then(() => toggleNFC(false))
  .then(endTest)
  .catch(handleRejectedPromise);
}






function testNfcConnectError() {
  log('testNfcConnectError');
  toggleNFC(true)
  .then(() => NCI.activateRE(emulator.P2P_RE_INDEX_0))
  .then(registerAndFireOnpeerready)
  .then(() => connectToNFCTagExpectError(sessionTokens[0],
                                         'NDEF',
                                         'NfcConnectError'))
  .then(() => toggleNFC(false))
  .then(endTest)
  .catch(handleRejectedPromise);
}






function testNoErrorInTechMsg() {
  log('testNoErrorInTechMsg');

  let techDiscoveredHandler = function(msg) {
    ok('Message handler for nfc-manager-tech-discovered');
    is(msg.type, 'techDiscovered');
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
  sessionTokens = [];
  runNextTest();
}

function handleRejectedPromise() {
  ok(false, 'Handling rejected promise');
  toggleNFC(false).then(endTest);
}

function registerAndFireOnpeerready() {
  let deferred = Promise.defer();

  nfc.onpeerready = function(event) {
    sessionTokens.push(event.detail);
    nfcPeers.push(nfc.getNFCPeer(event.detail));
    nfc.onpeerready = null;
    deferred.resolve();
  };

  let req = nfc.checkP2PRegistration(MANIFEST_URL);
  req.onsuccess = function() {
    is(req.result, true, 'P2P registration result');
    if(req.result) {
      nfc.notifyUserAcceptedP2P(MANIFEST_URL);
    } else {
      ok(false, 'this should not happen');
      nfc.onpeerready = null;
      deferred.reject();
    }
  };

  req.onerror = function() {
    ok(false, 'not possible');
    nfc.onpeerready = null;
    deferred.reject();
  };

  return deferred.promise;
}

function sendNDEFExpectError(peer, errorMsg) {
  let deferred = Promise.defer();

  let req = peer.sendNDEF(NDEF_MESSAGE);
  req.onsuccess = function() {
    ok(false, 'success on sending ndef not possible shoudl get: ' + errorMsg);
    deferred.reject();
  };

  req.onerror = function() {
    ok(true, 'this should happen');
    is(req.error.name, errorMsg, 'Should have proper error name');
    deferred.resolve();
  };

  return deferred.promise;
}

function connectToNFCTagExpectError(sessionToken, tech, errorMsg) {
  let deferred = Promise.defer();

  let nfcTag = nfc.getNFCTag(sessionTokens[0]);
  let req = nfcTag.connect(tech);
  req.onsuccess = function() {
    ok(false, 'we should not be able to connect to the tag');
    deferred.reject();
  };

  req.onerror = function() {
    ok(true, 'we should get an error');
    is(req.error.name, errorMsg, 'Should have proper error name');
    deferred.resolve();
  };

  return deferred.promise;
}

function setAndFireTechLostHandler() {
  let deferred = Promise.defer();

  let techLostHandler = function(msg) {
    ok('Message handler for nfc-manager-tech-lost');
    is(msg.type, 'techLost');
    is(msg.errorMsg, undefined, 'Should not get error msg in tech lost');

    deferred.resolve();
  };

  sysMsgHelper.waitForTechLost(techLostHandler);

  
  NCI.deactivate();
  return deferred.promise;
}

let tests = [
  testNfcNotEnabledError,
  testNfcBadSessionIdError,
  testNfcConnectError,
  testNoErrorInTechMsg
];







SpecialPowers.pushPermissions(
  [
    {'type': 'nfc-manager', 'allow': true, context: document},
    {'type': 'nfc-write', 'allow': true, context: document}
  ], runTests);
