


let pendingEmulatorCmdCount = 0;

function toggleNFC(enabled, callback) {
  isnot(callback, null);
  var settings = window.navigator.mozSettings;
  isnot(settings, null);
  ok(settings instanceof SettingsManager,
     'settings instanceof ' + settings.constructor +
     ', expected SettingsManager');

  let req = settings.createLock().get('nfc.enabled');
  req.onsuccess = function() {
    if (req.result['nfc.enabled'] === enabled) {
      callback();
    } else {
      let req = settings.createLock().set({'nfc.enabled': enabled});
      req.onsuccess = function() {
        window.setTimeout(callback, 5000); 
      };
      req.onerror = function() {
        ok(false,
           'Setting \'nfc.enabled\' to \'' + enabled +
           '\' failed, error ' + req.error.name);
        finish();
      };
    }
  };
  req.onerror = function() {
    ok(false, 'Getting \'nfc.enabled\' failed, error ' + req.error.name);
    finish();
  };
}

function cleanUp() {
  log('Cleaning up');
  waitFor(finish(),
          function() {
            return pendingEmulatorCmdCount === 0;
          });
}

function runNextTest() {
  let test = tests.shift();
  if (!test) {
    cleanUp();
    return;
  }
  test();
}


function runTests() {
  if ('mozNfc' in window.navigator) {
    runNextTest();
  } else {
    
    log('Skipping test on system without NFC');
    ok(true, 'Skipping test on system without NFC');
    finish();
  }
}
