


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function testGetCallBarringOption(aExpectedError, aOptions) {
  log("Test getCallBarringOption with " + JSON.stringify(aOptions));

  return getCallBarringOption(aOptions)
    .then(function resolve(aResult) {
      ok(false, "should be rejected");
    }, function reject(aError) {
      is(aError.name, aExpectedError, "failed to getCallBarringOption");
    });
}


startTestCommon(function() {
  return Promise.resolve()

    
    .then(() => testGetCallBarringOption("InvalidParameter", {
      program: 5, 
      serviceClass: 0
    }))

    .then(() => testGetCallBarringOption("InvalidParameter", {
      program: null, 
      serviceClass: 0
    }))

    .then(() => testGetCallBarringOption("InvalidParameter", {
      
      serviceClass: 0
    }))

    
    .then(() => testGetCallBarringOption("InvalidParameter", {
      program: MozMobileConnection.CALL_BARRING_PROGRAM_ALL_OUTGOING,
      serviceClass: null 
    }))

    .then(() => testGetCallBarringOption("InvalidParameter", {
      program: MozMobileConnection.CALL_BARRING_PROGRAM_ALL_OUTGOING,
      
    }));
});
