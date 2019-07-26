


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  
  {
    options: {
      "program": 5, 
      "enabled": true,
      "password": "0000",
      "serviceClass": 0
    },
    expectedError: "InvalidParameter"
  }, {
    options: {
      "program": null,
      "enabled": true,
      "password": "0000",
      "serviceClass": 0
    },
    expectedError: "InvalidParameter"
  }, {
    options: {
      
      "enabled": true,
      "password": "0000",
      "serviceClass": 0
    },
    expectedError: "InvalidParameter"
  },
  
  {
    options: {
      "program": MozMobileConnection.CALL_BARRING_PROGRAM_ALL_OUTGOING,
      "enabled": null,
      "password": "0000",
      "serviceClass": 0
    },
    expectedError: "InvalidParameter"
  }, {
    options: {
      "program": MozMobileConnection.CALL_BARRING_PROGRAM_ALL_OUTGOING,
      
      "password": "0000",
      "serviceClass": 0
    },
    expectedError: "InvalidParameter"
  },
  
  {
    options: {
      "program": MozMobileConnection.CALL_BARRING_PROGRAM_ALL_OUTGOING,
      "enabled": true,
      "password": null,
      "serviceClass": 0
    },
    expectedError: "InvalidParameter"
  }, {
    options: {
      "program": MozMobileConnection.CALL_BARRING_PROGRAM_ALL_OUTGOING,
      "enabled": true,
      
      "serviceClass": 0
    },
    expectedError: "InvalidParameter"
  },
  
  {
    options: {
      "program": MozMobileConnection.CALL_BARRING_PROGRAM_ALL_OUTGOING,
      "enabled": true,
      "password": "0000",
      "serviceClass": null
    },
    expectedError: "InvalidParameter"
  }, {
    options: {
      "program": MozMobileConnection.CALL_BARRING_PROGRAM_ALL_OUTGOING,
      "enabled": true,
      "password": "0000",
      
    },
    expectedError: "InvalidParameter"
  },
  
  
  
  {
    options: {
      "program": MozMobileConnection.CALL_BARRING_PROGRAM_ALL_OUTGOING,
      "enabled": true,
      "password": "0000",
      "serviceClass": 0
    },
    expectedError: "RequestNotSupported"
  }
];

function testSetCallBarringOption(aOptions, aExpectedError) {
  log("Test setting call barring to " + JSON.stringify(aOptions));

  return setCallBarringOption(aOptions)
    .then(function resolve() {
      ok(false, "changeCallBarringPassword success");
    }, function reject(aError) {
      is(aError.name, aExpectedError, "failed to changeCallBarringPassword");
    });
}


startTestCommon(function() {
  let promise = Promise.resolve();
  for (let i = 0; i < TEST_DATA.length; i++) {
    let data = TEST_DATA[i];
    promise = promise.then(() => testSetCallBarringOption(data.options,
                                                          data.expectedError));
  }
  return promise;
});
