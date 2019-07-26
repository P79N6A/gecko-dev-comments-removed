


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function testSetCallingLineIdRestriction(aMode, aExpectedErrorMsg) {
  log("Test setting calling line id restriction mode to " + aMode);

  return setClir(aMode)
    .then(function resolve() {
      ok(!aExpectedErrorMsg, "setCallingLineIdRestriction success");
    }, function reject(aError) {
      is(aError.name, aExpectedErrorMsg,
         "failed to setCallingLineIdRestriction");
    });
}




function testGetCallingLineIdRestriction() {
  log("Test getting calling line id restriction mode");

  return getClir()
    .then(function resolve() {
      ok(false, "getCallingLineIdRestriction should not success");
    }, function reject(aError) {
      is(aError.name, "RequestNotSupported",
         "failed to getCallingLineIdRestriction");
    });
}


startTestCommon(function() {
  return Promise.resolve()
    
    
    
    
    .then(() => testSetCallingLineIdRestriction(
                  MozMobileConnection.CLIR_DEFAULT, "RequestNotSupported"))
    .then(() => testSetCallingLineIdRestriction(
                  MozMobileConnection.CLIR_INVOCATION, "RequestNotSupported"))
    .then(() => testSetCallingLineIdRestriction(
                  MozMobileConnection.CLIR_SUPPRESSION, "RequestNotSupported"))

    
    .then(() => testSetCallingLineIdRestriction(10 ,
                                                "InvalidParameter"))

    
    .then(() => testGetCallingLineIdRestriction());
});
