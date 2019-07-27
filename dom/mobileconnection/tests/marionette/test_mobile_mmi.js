


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function testGettingIMEI() {
  log("Test *#06# ...");

  let MMI_CODE = "*#06#";
  return sendMMI(MMI_CODE)
    .then(function resolve(aResult) {
      ok(true, MMI_CODE + " success");
      is(aResult.serviceCode, "scImei", "Service code IMEI");
      
      
      
      is(aResult.statusMessage, "000000000000000", "Emulator IMEI");
      is(aResult.additionalInformation, undefined, "No additional information");
    }, function reject() {
      ok(false, MMI_CODE + " should not fail");
    });
}

function testInvalidMMICode() {
  log("Test invalid MMI code ...");

  let MMI_CODE = "InvalidMMICode";
  return sendMMI(MMI_CODE)
    .then(function resolve() {
      ok(false, MMI_CODE + " should not success");
    }, function reject(aError) {
      ok(true, MMI_CODE + " fail");
      is(aError.name, "emMmiError", "MMI error name");
      is(aError.message, "", "No message");
      is(aError.serviceCode, "scUssd", "Service code USSD");
      is(aError.additionalInformation, null, "No additional information");
    });
}


startTestCommon(function() {
   return Promise.resolve()
    .then(() => testGettingIMEI())
    .then(() => testInvalidMMICode());
});
