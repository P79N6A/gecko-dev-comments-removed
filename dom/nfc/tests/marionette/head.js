


let pendingEmulatorCmdCount = 0;

let Promise =
  SpecialPowers.Cu.import("resource://gre/modules/Promise.jsm").Promise;
let nfc = window.navigator.mozNfc;

SpecialPowers.addPermission("nfc-manager", true, document);




let emulator = (function() {
  let pendingCmdCount = 0;
  let originalRunEmulatorCmd = runEmulatorCmd;

  
  runEmulatorCmd = function() {
    throw "Use emulator.run(cmd, callback) instead of runEmulatorCmd";
  };

  function run(cmd, callback) {
    pendingCmdCount++;
    originalRunEmulatorCmd(cmd, function(result) {
      pendingCmdCount--;
      if (callback && typeof callback === "function") {
        callback(result);
      }
    });
  }

  return {
    run: run
  };
}());

function toggleNFC(enabled, callback) {
  isnot(callback, null);

  let req;
  if (enabled) {
    req = nfc.startPoll();
  } else {
    req = nfc.powerOff();
  }

  req.onsuccess = function() {
    callback();
  };

  req.onerror = function() {
    ok(false, 'operation failed, error ' + req.error.name);
    finish();
  };
}

function cleanUp() {
  log('Cleaning up');
  waitFor(function() {
            SpecialPowers.removePermission("nfc-manager", document);
            finish()
          },
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

const NDEF = {
  TNF_WELL_KNOWN: 1,

  
  compare: function(ndef1, ndef2) {
    isnot(ndef1, null, "LHS message is not null");
    isnot(ndef2, null, "RHS message is not null");
    is(ndef1.length, ndef2.length,
       "NDEF messages have the same number of records");
    ndef1.forEach(function(record1, index) {
      let record2 = this[index];
      is(record1.tnf, record2.tnf, "test for equal TNF fields");
      let fields = ["type", "id", "payload"];
      fields.forEach(function(value) {
        let field1 = record1[value];
        let field2 = record2[value];
        is(field1.length, field2.length,
           value + " fields have the same length");
        let eq = true;
        for (let i = 0; eq && i < field1.length; ++i) {
          eq = (field1[i] === field2[i]);
        }
        ok(eq, value + " fields contain the same data");
      });
    }, ndef2);
  },

  
  parseString: function(str) {
    
    let arr = null;
    try {
      arr = JSON.parse(str);
    } catch (e) {
      ok(false, "Parser error: " + e.message);
      return null;
    }
    
    let ndef = arr.map(function(value) {
        let type = new Uint8Array(NfcUtils.fromUTF8(this.atob(value.type)));
        let id = new Uint8Array(NfcUtils.fromUTF8(this.atob(value.id)));
        let payload =
          new Uint8Array(NfcUtils.fromUTF8(this.atob(value.payload)));
        return new MozNDEFRecord(value.tnf, type, id, payload);
      }, window);
    return ndef;
  }
};

var NfcUtils = {
  fromUTF8: function(str) {
    let buf = new Uint8Array(str.length);
    for (let i = 0; i < str.length; ++i) {
      buf[i] = str.charCodeAt(i);
    }
    return buf;
  }
};
