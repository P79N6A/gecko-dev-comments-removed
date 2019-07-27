


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function testGettingIMEI() {
  log("Test *#06# ...");

  return gSendMMI("*#06#").then(aResult => {
    ok(aResult.success, "success");
    is(aResult.serviceCode, "scImei", "Service code IMEI");
    
    
    
    is(aResult.statusMessage, "000000000000000", "Emulator IMEI");
    is(aResult.additionalInformation, undefined, "No additional information");
  });
}


startTest(function() {
  testGettingIMEI()
    .catch(error => ok(false, "Promise reject: " + error))
    .then(finish);
});
