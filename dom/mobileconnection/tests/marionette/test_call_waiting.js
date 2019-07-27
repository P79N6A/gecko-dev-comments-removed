


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";


startTestCommon(function() {
  return Promise.resolve()
    
    
    
    
    .then(() => setCallWaitingOption(true))
    .then(() => {
      ok(false, "setCallWaitingOption should not success");
    }, aError => {
      is(aError.name, "RequestNotSupported",
         "failed to setCallWaitingOption");
    })

    .then(() => getCallWaitingOption())
    .then(() => {
      ok(false, "getCallWaitingOption should not success");
    }, aError => {
      is(aError.name, "RequestNotSupported",
         "failed to getCallWaitingOption");
    });
});
