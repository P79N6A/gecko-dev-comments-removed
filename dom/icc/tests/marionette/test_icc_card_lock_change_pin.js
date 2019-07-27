


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const LOCK_TYPE = "pin";

function testChangePin(aIcc, aPin, aNewPin, aErrorName, aRetryCount) {
  log("testChangePin for pin=" + aPin + " and newPin=" + aNewPin);
  return aIcc.setCardLock({ lockType: LOCK_TYPE, pin: aPin, newPin: aNewPin })
    .then((aResult) => {
      if (aErrorName) {
        ok(false, "changing pin should not success");
      }
    }, (aError) => {
      if (!aErrorName) {
        ok(false, "changing pin should not fail");
        return;
      }

      
      is(aError.name, aErrorName, "error.name");
      is(aError.retryCount, aRetryCount, "error.retryCount");
    });
}


startTestCommon(function() {
  let icc = getMozIcc();
  let retryCount;

  return icc.getCardLockRetryCount(LOCK_TYPE)
    
    .then((aResult) => {
      retryCount = aResult.retryCount;
      ok(true, LOCK_TYPE + " retryCount is " + retryCount);
    })
    
    
    .then(() => testChangePin(icc, "1111", DEFAULT_PIN, "IncorrectPassword",
                              retryCount - 1))
    
    .then(() => testChangePin(icc, DEFAULT_PIN, DEFAULT_PIN));
});
