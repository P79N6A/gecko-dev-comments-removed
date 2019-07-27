


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function testUnlockPin(aIcc, aPin, aErrorName, aRetryCount) {
  log("testUnlockPin with pin=" + aPin);

  return aIcc.unlockCardLock({ lockType: "pin", pin: aPin })
    .then((aResult) => {
      if (aErrorName) {
        ok(false, "unlocking pin should not success");
      }
    }, (aError) => {
      if (!aErrorName) {
        ok(false, "unlocking pin should not fail");
        return;
      }

      
      is(aError.name, aErrorName, "error.name");
      is(aError.retryCount, aRetryCount, "error.retryCount");
    });
}

function testUnlockPinAndWait(aIcc, aPin, aCardState) {
  log("testUnlockPin with pin=" + aPin + ", and wait cardState changes to '" +
      aCardState + "'");

  let promises = [];

  promises.push(waitForTargetEvent(aIcc, "cardstatechange", function() {
    return aIcc.cardState === aCardState;
  }));
  promises.push(testUnlockPin(aIcc, aPin));

  return Promise.all(promises);
}


startTestCommon(function() {
  let icc = getMozIcc();
  let retryCount;

  
  return setPinLockEnabled(icc, true)
    
    .then(() => restartRadioAndWait("pinRequired"))
    .then(() => { icc = getMozIcc(); })

    
    .then(() => icc.getCardLockRetryCount("pin"))
    .then((aResult) => {
      retryCount = aResult.retryCount;
      ok(true, "pin retryCount is " + retryCount);
    })

    
    
    .then(() => testUnlockPin(icc, "1111", "IncorrectPassword", retryCount - 1))

    
    .then(() => testUnlockPinAndWait(icc, DEFAULT_PIN, "ready"))

    
    .then(() => setPinLockEnabled(icc, false));
});
