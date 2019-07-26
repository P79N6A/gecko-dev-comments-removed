


MARIONETTE_TIMEOUT = 30000;

SpecialPowers.addPermission("mobileconnection", true, document);



let ifr = document.createElement("iframe");
let connection;
ifr.onload = function() {
  connection = ifr.contentWindow.navigator.mozMobileConnections[0];
  ok(connection instanceof ifr.contentWindow.MozMobileConnection,
     "connection is instanceof " + connection.constructor);

  
  
  is(connection.iccId, 89014103211118510720);

  runNextTest();
};
document.body.appendChild(ifr);

function waitForIccChange(callback) {
  connection.addEventListener("iccchange", function handler() {
    connection.removeEventListener("iccchange", handler);
    callback();
  });
}

function setRadioEnabled(enabled) {
  let request  = connection.setRadioEnabled(enabled);

  request.onsuccess = function onsuccess() {
    log('setRadioEnabled: ' + enabled);
  };

  request.onerror = function onerror() {
    ok(false, "setRadioEnabled should be ok");
  };
}

function testIccChangeOnRadioPowerOff() {
  
  setRadioEnabled(false);

  waitForIccChange(function() {
    is(connection.iccId, null);
    runNextTest();
  });
}

function testIccChangeOnRadioPowerOn() {
  
  setRadioEnabled(true);

  waitForIccChange(function() {
    
    is(connection.iccId, 89014103211118510720);
    runNextTest();
  });
}

let tests = [
  testIccChangeOnRadioPowerOff,
  testIccChangeOnRadioPowerOn
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
  SpecialPowers.removePermission("mobileconnection", document);

  finish();
}
