


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function passingWrongPinAndWait(aIcc) {
  return aIcc.getCardLockRetryCount("pin").then((aResult) => {
    let promises = [];
    let retryCount = aResult.retryCount;

    ok(true, "pin retryCount is " + retryCount);

    promises.push(waitForTargetEvent(aIcc, "cardstatechange", function() {
      return aIcc.cardState === "pukRequired";
    }));

    for (let i = 0; i < retryCount; i++) {
      promises.push(aIcc.unlockCardLock({ lockType: "pin", pin: "1111" })
        .then(() => {
          ok(false, "unlocking pin should not success");
        }, function reject(aRetryCount, aError) {
          
          is(aError.name, "IncorrectPassword", "error.name");
          is(aError.retryCount, aRetryCount, "error.retryCount");
        }.bind(null, retryCount - i - 1)));
    }

    return Promise.all(promises);
  });
}

function testUnlockPuk(aIcc, aPuk, aNewPin, aErrorName, aRetryCount) {
  log("testUnlockPuk with puk=" + aPuk + " and newPin=" + aNewPin);

  return aIcc.unlockCardLock({ lockType: "puk", puk: aPuk, newPin: aNewPin })
    .then((aResult) => {
      if (aErrorName) {
        ok(false, "unlocking puk should not success");
      }
    }, (aError) => {
      if (!aErrorName) {
        ok(false, "unlocking puk should not fail");
        return;
      }

      
      is(aError.name, aErrorName, "error.name");
      is(aError.retryCount, aRetryCount, "error.retryCount");
    });
}

function testUnlockPukAndWait(aIcc, aPuk, aNewPin, aCardState) {
  log("testUnlockPuk with puk=" + aPuk + "/newPin=" + aNewPin +
      ", and wait card state changes to '" + aCardState + "'");

  let promises = [];

  promises.push(waitForTargetEvent(aIcc, "cardstatechange", function() {
    return aIcc.cardState === aCardState;
  }));
  promises.push(testUnlockPuk(aIcc, aPuk, aNewPin));

  return Promise.all(promises);
}


startTestCommon(function() {
  let icc = getMozIcc();
  let retryCount;

  
  return setPinLockEnabled(icc, true)
    
    .then(() => restartRadioAndWait("pinRequired"))
    .then(() => { icc = getMozIcc() })
    
    .then(() => passingWrongPinAndWait(icc))

    
    .then(() => icc.getCardLockRetryCount("puk"))
    .then((aResult) => {
      retryCount = aResult.retryCount;
      ok(true, "puk retryCount is " + retryCount);
    })

    
    
    .then(() => testUnlockPuk(icc, "11111111", DEFAULT_PIN, "IncorrectPassword",
                              retryCount - 1))

    
    .then(() => testUnlockPukAndWait(icc, DEFAULT_PUK, DEFAULT_PIN, "ready"))

    
    .then(() => setPinLockEnabled(icc, false));
});
