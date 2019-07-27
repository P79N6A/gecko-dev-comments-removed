


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


startTest(function() {
  Promise.resolve()
    .then(() => testGettingIMEI())
    .then(finish);
});
