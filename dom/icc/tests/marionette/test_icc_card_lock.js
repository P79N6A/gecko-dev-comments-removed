


MARIONETTE_TIMEOUT = 30000;

SpecialPowers.addPermission("mobileconnection", true, document);

let icc = navigator.mozIccManager;
ok(icc instanceof MozIccManager, "icc is instanceof " + icc.constructor);


function resetPinRetries(pin, callback) {
  let request = icc.setCardLock(
    {lockType: "pin",
     pin: pin,
     newPin: pin});

  request.onsuccess = function onsuccess() {
    callback();
  };

  request.onerror = function onerror() {
    is(false, "Reset pin retries got error: " + request.error.name);
    callback();
  };
}


function testPinChangeFailed() {
  
  let request = icc.setCardLock(
    {lockType: "pin",
     pin: "1111",
     newPin: "0000"});

  ok(request instanceof DOMRequest,
     "request instanceof " + request.constructor);

  request.onerror = function onerror() {
    is(request.error.name, "IncorrectPassword");

    resetPinRetries("0000", runNextTest);
  };
}


function testPinChangeFailedNotification() {
  icc.addEventListener("icccardlockerror", function onicccardlockerror(result) {
    icc.removeEventListener("icccardlockerror", onicccardlockerror);

    is(result.lockType, "pin");
    
    is(result.retryCount, 2);

    resetPinRetries("0000", runNextTest);
  });

  
  let request = icc.setCardLock(
    {lockType: "pin",
     pin: "1111",
     newPin: "0000"});

  ok(request instanceof DOMRequest,
     "request instanceof " + request.constructor);
}


function testPinChangeSuccess() {
  
  let request = icc.setCardLock(
    {lockType: "pin",
     pin: "0000",
     newPin: "0000"});

  ok(request instanceof DOMRequest,
     "request instanceof " + request.constructor);

  request.onerror = function onerror() {
    ok(false, "Should not fail, got error: " + request.error.name);

    runNextTest();
  };

  request.onsuccess = function onsuccess() {
    is(request.result.lockType, "pin");
    is(request.result.success, true);

    runNextTest();
  };
}

let tests = [
  testPinChangeFailed,
  testPinChangeFailedNotification,
  testPinChangeSuccess,
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
