


MARIONETTE_TIMEOUT = 30000;
MARIONETTE_HEAD_JS = 'head.js';


const NCI_LAST_NOTIFICATION  = 0;
const NCI_LIMIT_NOTIFICATION = 1;
const NCI_MORE_NOTIFICATIONS = 2;

function handleTechnologyDiscoveredRE0(msg) {
  log('Received \'nfc-manager-tech-discovered\'');
  is(msg.type, 'techDiscovered', 'check for correct message type');
  is(msg.techList[0], 'P2P', 'check for correct tech type');
  toggleNFC(false).then(runNextTest);
}

function activateRE(re) {
  let deferred = Promise.defer();
  let cmd = 'nfc nci rf_intf_activated_ntf ' + re;

  emulator.run(cmd, function(result) {
    is(result.pop(), 'OK', 'check activation of RE' + re);
    deferred.resolve();
  });

  return deferred.promise;
}

function notifyDiscoverRE(re, type) {
  let deferred = Promise.defer();
  let cmd = 'nfc nci rf_discover_ntf ' + re + ' ' + type;

  emulator.run(cmd, function(result) {
    is(result.pop(), 'OK', 'check discover of RE' + re);
    deferred.resolve();
  });

  return deferred.promise;
}

function testActivateRE0() {
  log('Running \'testActivateRE0\'');
  window.navigator.mozSetMessageHandler(
    'nfc-manager-tech-discovered', handleTechnologyDiscoveredRE0);

  toggleNFC(true).then(() => activateRE(0));
}



function testRfDiscover() {
  log('Running \'testRfDiscover\'');
  window.navigator.mozSetMessageHandler(
    'nfc-manager-tech-discovered', handleTechnologyDiscoveredRE0);

  toggleNFC(true)
  .then(() => notifyDiscoverRE(0, NCI_MORE_NOTIFICATIONS))
  .then(() => notifyDiscoverRE(1, NCI_LAST_NOTIFICATION))
  .then(() => activateRE(0));
}

let tests = [
  testActivateRE0,
  testRfDiscover
];

SpecialPowers.pushPermissions(
  [{'type': 'nfc-manager', 'allow': true, context: document}], runTests);
