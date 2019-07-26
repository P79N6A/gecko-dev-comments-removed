


MARIONETTE_TIMEOUT = 30000;

SpecialPowers.addPermission("mobileconnection", true, document);



let ifr = document.createElement("iframe");
let connection;
ifr.onload = function() {
  connection = ifr.contentWindow.navigator.mozMobileConnection;
  ok(connection instanceof ifr.contentWindow.MozMobileConnection,
     "connection is instanceof " + connection.constructor);
  testConnectionInfo();
};
document.body.appendChild(ifr);

let emulatorCmdPendingCount = 0;
function setEmulatorVoiceState(state) {
  emulatorCmdPendingCount++;
  runEmulatorCmd("gsm voice " + state, function (result) {
    emulatorCmdPendingCount--;
    is(result[0], "OK");
  });
}

function setEmulatorGsmLocation(lac, cid) {
  emulatorCmdPendingCount++;
  runEmulatorCmd("gsm location " + lac + " " + cid, function (result) {
    emulatorCmdPendingCount--;
    is(result[0], "OK");
  });
}

function testConnectionInfo() {
  let voice = connection.voice;
  is(voice.connected, true);
  is(voice.state, "registered");
  is(voice.emergencyCallsOnly, false);
  is(voice.roaming, false);

  testCellLocation();
}

function testCellLocation() {
  let cell = connection.voice.cell;

  
  
  
  ok(cell, "location available");

  
  is(cell.gsmLocationAreaCode, 65535);
  is(cell.gsmCellId, 268435455);
  is(cell.cdmaBaseStationId, -1);
  is(cell.cdmaBaseStationLatitude, -2147483648);
  is(cell.cdmaBaseStationLongitude, -2147483648);
  is(cell.cdmaSystemId, -1);
  is(cell.cdmaNetworkId, -1);

  connection.addEventListener("voicechange", function onvoicechange() {
    connection.removeEventListener("voicechange", onvoicechange);

    is(cell.gsmLocationAreaCode, 100);
    is(cell.gsmCellId, 100);
    is(cell.cdmaBaseStationId, -1);
    is(cell.cdmaBaseStationLatitude, -2147483648);
    is(cell.cdmaBaseStationLongitude, -2147483648);
    is(cell.cdmaSystemId, -1);
    is(cell.cdmaNetworkId, -1);

    testUnregistered();
  });

  setEmulatorGsmLocation(100, 100);
}

function testUnregistered() {
  setEmulatorVoiceState("unregistered");

  connection.addEventListener("voicechange", function onvoicechange() {
    connection.removeEventListener("voicechange", onvoicechange);

    is(connection.voice.connected, false);
    is(connection.voice.state, "notSearching");
    is(connection.voice.emergencyCallsOnly, false);
    is(connection.voice.roaming, false);
    is(connection.voice.cell, null);

    testSearching();
  });
}

function testSearching() {
  setEmulatorVoiceState("searching");

  connection.addEventListener("voicechange", function onvoicechange() {
    connection.removeEventListener("voicechange", onvoicechange);

    is(connection.voice.connected, false);
    is(connection.voice.state, "searching");
    is(connection.voice.emergencyCallsOnly, false);
    is(connection.voice.roaming, false);
    is(connection.voice.cell, null);

    testDenied();
  });
}

function testDenied() {
  setEmulatorVoiceState("denied");

  connection.addEventListener("voicechange", function onvoicechange() {
    connection.removeEventListener("voicechange", onvoicechange);

    is(connection.voice.connected, false);
    is(connection.voice.state, "denied");
    is(connection.voice.emergencyCallsOnly, false);
    is(connection.voice.roaming, false);
    is(connection.voice.cell, null);

    testRoaming();
  });
}

function testRoaming() {
  setEmulatorVoiceState("roaming");

  connection.addEventListener("voicechange", function onvoicechange() {
    connection.removeEventListener("voicechange", onvoicechange);

    is(connection.voice.connected, true);
    is(connection.voice.state, "registered");
    is(connection.voice.emergencyCallsOnly, false);
    is(connection.voice.roaming, true);

    testHome();
  });
}

function testHome() {
  setEmulatorVoiceState("home");

  connection.addEventListener("voicechange", function onvoicechange() {
    connection.removeEventListener("voicechange", onvoicechange);

    is(connection.voice.connected, true);
    is(connection.voice.state, "registered");
    is(connection.voice.emergencyCallsOnly, false);
    is(connection.voice.roaming, false);

    cleanUp();
  });
}

function cleanUp() {
  if (emulatorCmdPendingCount > 0) {
    setTimeout(cleanUp, 100);
    return;
  }

  SpecialPowers.removePermission("mobileconnection", document);
  finish();
}
