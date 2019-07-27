


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  
  
  
  ["*#31#", "RequestNotSupported"],
  
  
  ["*31#", "RequestNotSupported"],
  ["#31#", "RequestNotSupported"],
  
  ["**31#", "emMmiErrorNotSupported"],
  ["##31#", "emMmiErrorNotSupported"],
];

function testCLIR(aMmi, aExpectedError) {
  log("Test " + aMmi + " ...");

  return gSendMMI(aMmi).then(aResult => {
    
    
    ok(!aResult.success, "Check success");
    is(aResult.serviceCode, "scClir", "Check serviceCode");
    is(aResult.statusMessage, aExpectedError, "Check statusMessage");
  });
}


startTest(function() {
  let promise = Promise.resolve();

  TEST_DATA.forEach(function(aData) {
    promise = promise.then(() => testCLIR(aData[0], aData[1]));
  });

  return promise
    .catch(aError => ok(false, "Promise reject: " + aError))
    .then(finish);
});
