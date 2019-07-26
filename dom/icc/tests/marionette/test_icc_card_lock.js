


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
    is(request.error.lockType, "pin");
    
    is(request.error.retryCount, 2);

    resetPinRetries("0000", runNextTest);
  };
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


function testPinCardLockRetryCount() {
  let request = icc.getCardLockRetryCount('pin');

  ok(request instanceof DOMRequest,
     'request instanceof ' + request.constructor);

  request.onsuccess = function onsuccess() {
    is(request.result.lockType, 'pin',
        'lockType is ' + request.result.lockType);
    ok(request.result.retryCount >= 0,
        'retryCount is ' + request.result.retryCount);
    runNextTest();
  };
  request.onerror = function onerror() {
    
    
    
    is(request.error.name, 'RequestNotSupported',
        'error name is ' + request.error.name);
    runNextTest();
  };
}


function testPukCardLockRetryCount() {
  let request = icc.getCardLockRetryCount('puk');

  ok(request instanceof DOMRequest,
     'request instanceof ' + request.constructor);

  request.onsuccess = function onsuccess() {
    is(request.result.lockType, 'puk',
        'lockType is ' + request.result.lockType);
    ok(request.result.retryCount >= 0,
        'retryCount is ' + request.result.retryCount);
    runNextTest();
  };
  request.onerror = function onerror() {
    
    
    
    is(request.error.name, 'RequestNotSupported',
        'error name is ' + request.error.name);
    runNextTest();
  };
}


function testInvalidCardLockRetryCount() {
  let request = icc.getCardLockRetryCount('invalid-lock-type');

  ok(request instanceof DOMRequest,
     'request instanceof ' + request.constructor);

  request.onsuccess = function onsuccess() {
    ok(false,
        'request should never return success for an invalid lock type');
    runNextTest();
  };
  request.onerror = function onerror() {
    is(request.error.name, 'GenericFailure',
        'error name is ' + request.error.name);
    runNextTest();
  };
}

let tests = [
  testPinChangeFailed,
  testPinChangeSuccess,
  testPinCardLockRetryCount,
  testPukCardLockRetryCount,
  testInvalidCardLockRetryCount
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
