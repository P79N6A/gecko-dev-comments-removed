


MARIONETTE_TIMEOUT = 10000;

let Services = SpecialPowers.Services;

function testScreenState(on, expected, msg) {
  
  Services.obs.notifyObservers(null, 'screen-state-changed', on);
  
  window.setTimeout(function() {
    runEmulatorCmd('gsm report creg', function(result) {
      is(result.pop(), 'OK', '\'gsm report creg\' successful');
      ok(result.indexOf(expected) !== -1, msg);
      runNextTest();
    })}, 1000);
}

function testScreenStateDisabled() {
  testScreenState('off', '+CREG: 1', 'screen is disabled');
}

function testScreenStateEnabled() {
  testScreenState('on', '+CREG: 2', 'screen is enabled');
}

let tests = [
  testScreenStateDisabled,
  testScreenStateEnabled
];

function runNextTest() {
  let test = tests.shift();
  if (!test) {
    cleanUp();
    return;
  }

  test();
}

function cleanUp() {
  finish();
}

runNextTest();
