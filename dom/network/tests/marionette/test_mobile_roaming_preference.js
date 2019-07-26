


MARIONETTE_TIMEOUT = 60000;

SpecialPowers.addPermission("mobileconnection", true, document);

let connection = navigator.mozMobileConnection;
ok(connection instanceof MozMobileConnection,
   "connection is instanceof " + connection.constructor);

function failedToSetRoamingPreference(mode, expectedErrorMessage, callback) {
  let request = connection.setRoamingPreference(mode);

  ok(request instanceof DOMRequest,
     "request instanceof " + request.constructor);

  request.onsuccess = function onsuccess() {
    ok(false, "Should not be here !!");

    callback();
  }

  request.onerror = function onerror() {
    is(request.error.name, expectedErrorMessage);

    callback();
  }
}

function testSetRoamingPreferenceWithNullValue() {
  log("test setRoamingPreference(null)");

  failedToSetRoamingPreference(null, "InvalidParameter", runNextTest);
}

function testSetRoamingPreferenceWithInvalidValue() {
  log("test setRoamingPreference(\"InvalidValue\")");

  failedToSetRoamingPreference("InvalidValue", "InvalidParameter", runNextTest);
}

function testSetRoamingPreferenceToHome() {
  log("test setRoamingPreference(\"home\")");

  
  
  
  failedToSetRoamingPreference("home", "RequestNotSupported", runNextTest);
}

function testGetRoamingPreference() {
  log("test getRoamingPreference()");

  
  
  
  let request = connection.getRoamingPreference();

  ok(request instanceof DOMRequest,
     "request instanceof " + request.constructor);

  request.onsuccess = function onsuccess() {
    ok(false, "Should not be here !!");

    runNextTest();
  }

  request.onerror = function onerror() {
    is(request.error.name, "RequestNotSupported");

    runNextTest();
  }
}

let tests = [
  testSetRoamingPreferenceWithNullValue,
  testSetRoamingPreferenceWithInvalidValue,
  testSetRoamingPreferenceToHome,
  testGetRoamingPreference
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

runNextTest();