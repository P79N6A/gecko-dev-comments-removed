


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function restartRadioAndWait(aCardState) {
  let iccManager = getMozIccManager();
  return setRadioEnabledAndWait(false).then(() => {
    let promises = [];

    promises.push(waitForTargetEvent(iccManager, "iccdetected")
      .then((aEvent) => {
        let icc = getMozIccByIccId(aEvent.iccId);
        if (icc.cardState !== aCardState) {
          return waitForTargetEvent(icc, "cardstatechange", function() {
            return icc.cardState === aCardState;
          });
        }
      }));
    promises.push(setRadioEnabledAndWait(true));

    return Promise.all(promises);
  });
}

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
        }, (aError) => {
          ok(true, "pin retryCount = " + aError.retryCount);
        }));
    }

    return Promise.all(promises);
  });
}

function sendUnlockPukMmi(aPuk, aNewPin, aNewPinAgain) {
  let MMI_CODE = "**05*" + aPuk + "*" + aNewPin + "*" + aNewPinAgain + "#";
  log("Test " + MMI_CODE);

  return sendMMI(MMI_CODE);
}

function testUnlockPukMmiError(aPuk, aNewPin, aNewPinAgain, aErrorName,
                               aRetryCount = null) {
  return sendUnlockPukMmi(aPuk, aNewPin, aNewPinAgain)
    .then(() => {
      ok(false, "unlocking puk should not success");
    }, (aError) => {
      is(aError.name, aErrorName, "Check name");
      is(aError.message, "", "Check message");
      is(aError.serviceCode, "scPuk", "Check service code");
      is(aError.additionalInformation, aRetryCount,
         "Check additional information");
    });
}

function testUnlockPukAndWait(aIcc, aCardState) {
  let promises = [];

  promises.push(waitForTargetEvent(aIcc, "cardstatechange", function() {
    return aIcc.cardState === aCardState;
  }));
  promises.push(sendUnlockPukMmi(DEFAULT_PUK, DEFAULT_PIN, DEFAULT_PIN));

  return Promise.all(promises);
}


startTestCommon(function() {
  let icc = getMozIccByIccId();
  let retryCount;

  
  return icc.setCardLock({lockType: "pin", enabled: true, pin: DEFAULT_PIN})
    
    .then(() => restartRadioAndWait("pinRequired"))
    .then(() => { icc = getMozIccByIccId(); })
    
    .then(() => passingWrongPinAndWait(icc))

    
    .then(() => icc.getCardLockRetryCount("puk"))
    .then((aResult) => {
      retryCount = aResult.retryCount;
      ok(true, "puk retryCount is " + retryCount);
    })

    
    .then(() => testUnlockPukMmiError("", "1111", "2222", "emMmiError"))
    
    .then(() => testUnlockPukMmiError("11111111", "", "", "emMmiError"))
    
    .then(() => testUnlockPukMmiError("11111111", "1111", "2222",
                                      "emMmiErrorMismatchPin"))
    
    .then(() => testUnlockPukMmiError("123456789", DEFAULT_PIN, DEFAULT_PIN,
                                      "emMmiErrorInvalidPin"))
    
    .then(() => testUnlockPukMmiError("11111111", DEFAULT_PIN, DEFAULT_PIN,
                                      "emMmiErrorBadPuk", retryCount - 1))

    
    .then(() => testUnlockPukAndWait(icc, "ready"))

    
    .then(() => icc.setCardLock({lockType: "pin", enabled: false, pin: DEFAULT_PIN}));
});
