


MARIONETTE_TIMEOUT = 30000;
MARIONETTE_HEAD_JS = 'head.js';

let nfc = window.navigator.mozNfc;
function testEnableNFC() {
  log('Running \'testEnableNFC\'');
  let req = nfc.startPoll();
  req.onsuccess = function () {
    ok(true);
    runNextTest();
  };
  req.onerror = function () {
    ok(false, "startPoll failed");
    runNextTest();
  };
}

function testDisableNFC() {
  log('Running \'testDisableNFC\'');
  let req = nfc.powerOff();
  req.onsuccess = function () {
    ok(true);
    runNextTest();
  };
  req.onerror = function () {
    ok(false, "powerOff failed");
    runNextTest();
  };
}

function testStopPollNFC() {
  log('Running \'testStopPollNFC\'');
  let req = nfc.stopPoll();
  req.onsuccess = function () {
    ok(true);
    runNextTest();
  };
  req.onerror = function () {
    ok(false, "stopPoll failed");
    runNextTest();
  };
}

let tests = [
  testEnableNFC,
  testStopPollNFC,
  testDisableNFC
];

SpecialPowers.pushPermissions(
  [{'type': 'settings', 'allow': true, 'context': document},
   {'type': 'nfc-manager', 'allow': true, 'context': document}],
  runTests);
