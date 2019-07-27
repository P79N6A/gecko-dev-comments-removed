


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function testUSSD() {
  log("Test *#1234# ...");

  return gSendMMI("*#1234#").then(aResult => {
    
    
    ok(!aResult.success, "Check success");
    is(aResult.serviceCode, "scUssd", "Check serviceCode");
    is(aResult.statusMessage, "RequestNotSupported", "Check statusMessage");
    is(aResult.additionalInformation, undefined, "No additional information");
  });
}


startTest(function() {
  return testUSSD()
    .catch(error => ok(false, "Promise reject: " + error))
    .then(finish);
});
