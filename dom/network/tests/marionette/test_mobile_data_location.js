


MARIONETTE_TIMEOUT = 20000;

SpecialPowers.addPermission("mobileconnection", true, document);

let mobileConnection = navigator.mozMobileConnection;
let emulatorStartLac = 0;
let emulatorStartCid = 0;

function verifyInitialState() {
  log("Verifying initial state.");
  ok(mobileConnection instanceof MozMobileConnection,
      "mobileConnection is instanceof " + mobileConnection.constructor);
  testStartingCellLocation();
}

function testStartingCellLocation() {
  
  log("Getting the starting GSM location from the emulator.");

  runEmulatorCmd("gsm location", function(result) {
    log("Emulator callback.");
    is(result[0].substring(0,3), "lac", "lac output");
    is(result[1].substring(0,2), "ci", "ci output");
    is(result[2], "OK", "emulator ok");

    emulatorStartLac = result[0].substring(5);
    log("Emulator GSM location LAC is '" + emulatorStartLac + "'.");
    emulatorStartCid = result[1].substring(4);
    log("Emulator GSM location CID is '" + emulatorStartCid + "'.");

    log("mobileConnection.data.cell.gsmLocationAreaCode is '"
        + mobileConnection.data.cell.gsmLocationAreaCode + "'.");
    log("mobileConnection.data.cell.gsmCellId is '"
        + mobileConnection.data.cell.gsmCellId + "'.");

    
    if (emulatorStartLac == -1) {
      
      is(mobileConnection.data.cell.gsmLocationAreaCode,
          65535, "starting LAC");
    } else {
      
      is(mobileConnection.data.cell.gsmLocationAreaCode,
          emulatorStartLac, "starting LAC");
    }
    if (emulatorStartCid == -1) {
      
      is(mobileConnection.data.cell.gsmCellId, 268435455, "starting CID");
    } else {
      
      is(mobileConnection.data.cell.gsmCellId,
          emulatorStartCid, "starting CID");
    }

    
    testChangeCellLocation(emulatorStartLac, emulatorStartCid);
  });
}

function testChangeCellLocation() {
  
  let newLac = 1000;
  let newCid = 2000;
  let gotCallback = false;

  
  if (newLac == emulatorStartLac) { newLac++; };
  if (newCid == emulatorStartCid) { newCid++; };

  
  mobileConnection.addEventListener("datachange", function ondatachange() {
    mobileConnection.removeEventListener("datachange", ondatachange);
    log("Received 'ondatachange' event.");
    log("mobileConnection.data.cell.gsmLocationAreaCode is now '"
        + mobileConnection.data.cell.gsmLocationAreaCode + "'.");
    log("mobileConnection.data.cell.gsmCellId is now '"
        + mobileConnection.data.cell.gsmCellId + "'.");
    is(mobileConnection.data.cell.gsmLocationAreaCode, newLac,
        "data.cell.gsmLocationAreaCode");
    is(mobileConnection.data.cell.gsmCellId, newCid, "data.cell.gsmCellId");
    waitFor(restoreLocation, function() {
      return(gotCallback);
    });
  });

  
  log("Changing emulator GSM location to '" + newLac + ", " + newCid
      + "' and waiting for 'ondatachange' event.");
  gotCallback = false;
  runEmulatorCmd("gsm location " + newLac + " " + newCid, function(result) {
    is(result[0], "OK");
    log("Emulator callback on location change.");
    gotCallback = true;
  });
}

function restoreLocation() {
  
  log("Restoring emulator GSM location back to '" + emulatorStartLac + ", "
      + emulatorStartCid + "'.");
  runEmulatorCmd("gsm location " + emulatorStartLac + " " + emulatorStartCid,
      function(result) {
    log("Emulator callback on restore.");
    is(result[0], "OK");
    cleanUp();
  });
}

function cleanUp() {
  mobileConnection.ondatachange = null;
  SpecialPowers.removePermission("mobileconnection", document);
  finish();
}


verifyInitialState();
