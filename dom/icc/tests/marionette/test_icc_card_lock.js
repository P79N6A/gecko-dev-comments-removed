


MARIONETTE_TIMEOUT = 30000;
MARIONETTE_HEAD_JS = "icc_header.js";


taskHelper.push(function testPinChangeFailed() {
  
  let request = icc.setCardLock(
    {lockType: "pin",
     pin: "1111",
     newPin: "0000"});

  ok(request instanceof DOMRequest,
     "request instanceof " + request.constructor);

  request.onerror = function onerror() {
    is(request.error.name, "IncorrectPassword");
    
    is(request.error.retryCount, 2);

    
    let resetRequest = icc.setCardLock(
      {lockType: "pin",
       pin: "0000",
       newPin: "0000"});

    resetRequest.onsuccess = function onsuccess() {
      taskHelper.runNext();
    };

    resetRequest.onerror = function onerror() {
      ok(false, "Reset pin retries got error: " + request.error.name);
      taskHelper.runNext();
    };
  };
});


taskHelper.push(function testPinChangeSuccess() {
  
  let request = icc.setCardLock(
    {lockType: "pin",
     pin: "0000",
     newPin: "0000"});

  ok(request instanceof DOMRequest,
     "request instanceof " + request.constructor);

  request.onerror = function onerror() {
    ok(false, "Should not fail, got error: " + request.error.name);

    taskHelper.runNext();
  };

  request.onsuccess = function onsuccess() {
    taskHelper.runNext();
  };
});


taskHelper.push(function testPinCardLockRetryCount() {
  let request = icc.getCardLockRetryCount('pin');

  ok(request instanceof DOMRequest,
     'request instanceof ' + request.constructor);

  request.onsuccess = function onsuccess() {
    ok(request.result.retryCount >= 0,
        'retryCount is ' + request.result.retryCount);
    taskHelper.runNext();
  };
  request.onerror = function onerror() {
    
    
    
    is(request.error.name, 'RequestNotSupported',
        'error name is ' + request.error.name);
    taskHelper.runNext();
  };
});


taskHelper.push(function testPukCardLockRetryCount() {
  let request = icc.getCardLockRetryCount('puk');

  ok(request instanceof DOMRequest,
     'request instanceof ' + request.constructor);

  request.onsuccess = function onsuccess() {
    ok(request.result.retryCount >= 0,
        'retryCount is ' + request.result.retryCount);
    taskHelper.runNext();
  };
  request.onerror = function onerror() {
    
    
    
    is(request.error.name, 'RequestNotSupported',
        'error name is ' + request.error.name);
    taskHelper.runNext();
  };
});


taskHelper.push(function testInvalidCardLockRetryCount() {
  let request = icc.getCardLockRetryCount('invalid-lock-type');

  ok(request instanceof DOMRequest,
     'request instanceof ' + request.constructor);

  request.onsuccess = function onsuccess() {
    ok(false,
        'request should never return success for an invalid lock type');
    taskHelper.runNext();
  };
  request.onerror = function onerror() {
    is(request.error.name, 'GenericFailure',
        'error name is ' + request.error.name);
    taskHelper.runNext();
  };
});


taskHelper.runNext();
