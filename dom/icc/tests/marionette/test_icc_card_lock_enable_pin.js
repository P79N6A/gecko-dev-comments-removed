


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function setCardLockAndCheck(aIcc, aLockType, aPin, aEnabled, aErrorName,
                             aRetryCount) {
  let options = {
    lockType: aLockType,
    enabled: aEnabled,
    pin: aPin
  };

  return aIcc.setCardLock(options)
    .then((aResult) => {
      if (aErrorName) {
        ok(false, "setting pin should not success");
        return;
      }

      
      return aIcc.getCardLock(aLockType)
        .then((aResult) => {
          is(aResult.enabled, aEnabled, "result.enabled");
        });
    }, (aError) => {
      if (!aErrorName) {
        ok(false, "setting pin should not fail");
        return;
      }

      
      is(aError.name, aErrorName, "error.name");
      is(aError.retryCount, aRetryCount, "error.retryCount");
    });
}


startTestCommon(function() {
  let icc = getMozIcc();
  let lockType = "pin";
  let retryCount;

  return icc.getCardLockRetryCount(lockType)
    
    .then((aResult) => {
      retryCount = aResult.retryCount;
      ok(true, lockType + " retryCount is " + retryCount);
    })
    
    
    .then(() => setCardLockAndCheck(icc, lockType, "1111", true,
                                    "IncorrectPassword", retryCount -1))
    
    .then(() => setCardLockAndCheck(icc, lockType, DEFAULT_PIN, true))
    
    .then(() => setCardLockAndCheck(icc, lockType, DEFAULT_PIN, false));
});
