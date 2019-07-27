


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  
  
  
  ["*#30#", "RequestNotSupported"],
  
  ["*30#", "emMmiErrorNotSupported"],
  ["#30#", "emMmiErrorNotSupported"],
  ["**30#", "emMmiErrorNotSupported"],
  ["##30#", "emMmiErrorNotSupported"],
];

function testCLIP(aMmi, aExpectedError) {
  log("Test " + aMmi + " ...");

  return gSendMMI(aMmi).then(aResult => {
    
    
    ok(!aResult.success, "Check success");
    is(aResult.serviceCode, "scClip", "Check serviceCode");
    is(aResult.statusMessage, aExpectedError, "Check statusMessage");
  });
}


startTest(function() {
  let promise = Promise.resolve();

  TEST_DATA.forEach(function(aData) {
    promise = promise.then(() => testCLIP(aData[0], aData[1]));
  });

  return promise
    .catch(aError => ok(false, "Promise reject: " + aError))
    .then(finish);
});
