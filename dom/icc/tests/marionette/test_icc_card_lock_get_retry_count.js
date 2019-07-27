


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function testGetCardLockRetryCount(aIcc, aLockType, aRetryCount) {
  log("testGetCardLockRetryCount for " + aLockType);
  return aIcc.getCardLockRetryCount(aLockType)
    .then((aResult) => {
      if (!aRetryCount) {
        ok(false, "getCardLockRetryCount(" + aLockType + ") should not success");
        return;
      }

      
      is(aResult.retryCount, aRetryCount, "result.retryCount");
    }, (aError) => {
      if (aRetryCount) {
        ok(false, "getCardLockRetryCount(" + aLockType + ") should not fail");
        return;
      }

      
      is(aError.name, "GenericFailure", "error.name");
    });
}


startTestCommon(function() {
  let icc = getMozIcc();

  
  
  return testGetCardLockRetryCount(icc, "pin", 3)
    
    
    .then(() => testGetCardLockRetryCount(icc, "puk", 6))
    
    .then(() => testGetCardLockRetryCount(icc, "invalid-lock-type"));
});
