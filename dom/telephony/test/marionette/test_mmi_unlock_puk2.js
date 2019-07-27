


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function sendUnlockPuk2Mmi(aPuk2, aNewPin2, aNewPin2Again) {
  let MMI_CODE = "**052*" + aPuk2 + "*" + aNewPin2 + "*" + aNewPin2Again + "#";
  log("Test " + MMI_CODE);

  return gSendMMI(MMI_CODE);
}

function testUnlockPuk2MmiError(aPuk2, aNewPin2, aNewPin2Again, aErrorName) {
  return sendUnlockPuk2Mmi(aPuk2, aNewPin2, aNewPin2Again)
    .then((aResult) => {
      ok(!aResult.success, "Check success");
      is(aResult.serviceCode, "scPuk2", "Check service code");
      is(aResult.statusMessage, aErrorName, "Check statusMessage");
      is(aResult.additionalInformation, null, "Check additional information");
    });
}


startTest(function() {
  return Promise.resolve()
    
    .then(() => testUnlockPuk2MmiError("", "1111", "2222", "emMmiError"))
    
    .then(() => testUnlockPuk2MmiError("11111111", "", "", "emMmiError"))
    
    .then(() => testUnlockPuk2MmiError("11111111", "1111", "2222",
                                       "emMmiErrorMismatchPin"))
    
    .then(() => testUnlockPuk2MmiError("123456789", "0000", "0000",
                                       "emMmiErrorInvalidPin"))
    
    
    
    .then(() => testUnlockPuk2MmiError("11111111", "0000", "0000",
                                       "RequestNotSupported"))
    .catch(error => ok(false, "Promise reject: " + error))
    .then(finish);
});
