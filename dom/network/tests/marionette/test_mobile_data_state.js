


MARIONETTE_TIMEOUT = 30000;

SpecialPowers.addPermission("mobileconnection", true, document);

let mobileConnection = navigator.mozMobileConnection;

function verifyInitialState() {
  log("Verifying initial state.");
  ok(mobileConnection instanceof MozMobileConnection,
      "mobileConnection is instanceof " + mobileConnection.constructor);
  
  
  log("Starting mobileConnection.data.state is: '"
      + mobileConnection.data.state + "'.");
  if (mobileConnection.data.state != "registered") {
    changeDataStateAndVerify("home", "registered", testUnregistered);
  } else {
    testUnregistered();
  }
}

function changeDataStateAndVerify(dataState, expected, nextFunction) {
  let gotCallback = false;

  
  log("Changing emulator data state to '" + dataState
      + "' and waiting for 'ondatachange' event.");

  
  mobileConnection.addEventListener("datachange", function ondatachange() {
    mobileConnection.removeEventListener("datachange", ondatachange);
    log("Received 'ondatachange' event.");
    log("mobileConnection.data.state is now '"
        + mobileConnection.data.state + "'.");
    is(mobileConnection.data.state, expected, "data.state");
    waitFor(nextFunction, function() {
      return(gotCallback);
    });
  });

  
  gotCallback = false;
  runEmulatorCmd("gsm data " + dataState, function(result) {
    is(result[0], "OK");
    log("Emulator callback complete.");
    gotCallback = true;
  });
}

function testUnregistered() {
  log("Test 1: Unregistered.");
  
  
  changeDataStateAndVerify("unregistered", "notSearching", testRoaming);
}

function testRoaming() {
  log("Test 2: Roaming.");
  
  
  changeDataStateAndVerify("roaming", "registered", testOff);
}

function testOff() {
  log("Test 3: Off.");
  
  
  changeDataStateAndVerify("off", "notSearching", testSearching);
}

function testSearching() {
  log("Test 4: Searching.");
  

  
  

  
  log("* When Bug 819533 is fixed, change this test to expect 'searching' *");
  changeDataStateAndVerify("searching", "registered", testDenied);
}

function testDenied() {
  log("Test 5: Denied.");
  
  
  changeDataStateAndVerify("denied", "denied", testOn);
}

function testOn() {
  log("Test 6: On.");
  
  
  changeDataStateAndVerify("on", "registered", testOffAgain);
}

function testOffAgain() {
  log("Test 7: Off again.");
  
  
  changeDataStateAndVerify("off", "notSearching", testHome);
}

function testHome() {
  log("Test 8: Home.");
  
  
  changeDataStateAndVerify("home", "registered", cleanUp);
}

function cleanUp() {
  mobileConnection.ondatachange = null;
  SpecialPowers.removePermission("mobileconnection", document);
  finish();
}


verifyInitialState();
