


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function testSetCallBarringOption(aExpectedError, aOptions) {
  log("Test setCallBarringOption with " + JSON.stringify(aOptions));

  return setCallBarringOption(aOptions)
    .then(function resolve() {
      ok(false, "should be rejected");
    }, function reject(aError) {
      is(aError.name, aExpectedError, "failed to changeCallBarringPassword");
    });
}


startTestCommon(function() {
  return Promise.resolve()

    
    .then(() => testSetCallBarringOption("InvalidParameter", {
      "program": 5, 
      "enabled": true,
      "password": "0000",
      "serviceClass": 0
    }))

    .then(() => testSetCallBarringOption("InvalidParameter", {
      "program": null,
      "enabled": true,
      "password": "0000",
      "serviceClass": 0
    }))

    .then(() => testSetCallBarringOption("InvalidParameter", {
      
      "enabled": true,
      "password": "0000",
      "serviceClass": 0
    }))

    
    .then(() => testSetCallBarringOption("InvalidParameter", {
      "program": MozMobileConnection.CALL_BARRING_PROGRAM_ALL_OUTGOING,
      "enabled": null, 
      "password": "0000",
      "serviceClass": 0
    }))

    .then(() => testSetCallBarringOption("InvalidParameter", {
      "program": MozMobileConnection.CALL_BARRING_PROGRAM_ALL_OUTGOING,
      
      "password": "0000",
      "serviceClass": 0
    }))

    
    .then(() => testSetCallBarringOption("InvalidParameter", {
      "program": MozMobileConnection.CALL_BARRING_PROGRAM_ALL_OUTGOING,
      "enabled": true,
      "password": null, 
      "serviceClass": 0
    }))

    .then(() => testSetCallBarringOption("InvalidParameter", {
      "program": MozMobileConnection.CALL_BARRING_PROGRAM_ALL_OUTGOING,
      "enabled": true,
      
      "serviceClass": 0
    }))

    .then(() => testSetCallBarringOption("IncorrectPassword", {
      "program": MozMobileConnection.CALL_BARRING_PROGRAM_ALL_OUTGOING,
      "enabled": true,
      "password": "1111", 
      "serviceClass": 0
    }))

    
    .then(() => testSetCallBarringOption("InvalidParameter", {
      "program": MozMobileConnection.CALL_BARRING_PROGRAM_ALL_OUTGOING,
      "enabled": true,
      "password": "0000",
      "serviceClass": null 
    }))

    .then(() => testSetCallBarringOption("InvalidParameter", {
      "program": MozMobileConnection.CALL_BARRING_PROGRAM_ALL_OUTGOING,
      "enabled": true,
      "password": "0000",
      
    }))
});
