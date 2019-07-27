


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const DEFAULT_PIN = "0000";
const DEFAULT_PUK = "12345678";

function getMozIcc() {
  let iccId = navigator.mozMobileConnections[0].iccId;
  return navigator.mozIccManager.getIccById(iccId);
}

function waitForTargetEvent(aEventTarget, aEventName, aMatchFun) {
  let deferred = Promise.defer();

  aEventTarget.addEventListener(aEventName, function onevent(aEvent) {
    if (!aMatchFun || aMatchFun(aEvent)) {
      aEventTarget.removeEventListener(aEventName, onevent);
      ok(true, "Event '" + aEventName + "' got.");
      deferred.resolve(aEvent);
    }
  });

  return deferred.promise;
}

function setRadioEnabledAndWait(aEnabled) {
  let promises = [];
  let connection = navigator.mozMobileConnections[0];

  promises.push(waitForTargetEvent(connection, "radiostatechange", function() {
    return connection.radioState === aEnabled ? "enabled" : "disabled";
  }));
  promises.push(connection.setRadioEnabled(aEnabled));

  return Promise.all(promises);
}

function restartRadioAndWait(aCardState) {
  let iccManager = navigator.mozIccManager;

  return setRadioEnabledAndWait(false).then(() => {
    let promises = [];

    promises.push(waitForTargetEvent(iccManager, "iccdetected")
      .then((aEvent) => {
        let icc = iccManager.getIccById(aEvent.iccId);
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

  return gSendMMI(MMI_CODE);
}

function testUnlockPukMmiError(aPuk, aNewPin, aNewPinAgain, aErrorName,
                               aRetryCount = null) {
  return sendUnlockPukMmi(aPuk, aNewPin, aNewPinAgain)
    .then((aResult) => {
      ok(!aResult.success, "check success");
      is(aResult.serviceCode, "scPuk", "Check service code");
      is(aResult.statusMessage, aErrorName, "Check statusMessage");
      is(aResult.additionalInformation, aRetryCount,
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


startTestWithPermissions(['mobileconnection'], function() {
  let icc = getMozIcc();
  let retryCount;

  
  return icc.setCardLock({lockType: "pin", enabled: true, pin: DEFAULT_PIN})
    
    .then(() => restartRadioAndWait("pinRequired"))
    .then(() => { icc = getMozIcc(); })
    
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

    
    .then(() => icc.setCardLock({lockType: "pin", enabled: false, pin: DEFAULT_PIN}))
    .catch((aError) => ok(false, "Promise reject " + aError))
    .then(finish);
});
