


MARIONETTE_TIMEOUT = 30000;
MARIONETTE_HEAD_JS = 'head.js';

function testEnableNFC() {
  log('Running \'testEnableNFC\'');
  toggleNFC(true, runNextTest);
}

function testDisableNFC() {
  log('Running \'testDisableNFC\'');
  toggleNFC(false, runNextTest);
}

let tests = [
  testEnableNFC,
  testDisableNFC
];

SpecialPowers.pushPermissions(
  [{'type': 'settings', 'allow': true, 'context': document}], runTests);
