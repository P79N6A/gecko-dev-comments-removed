


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  
  {
    pin2: "",
    newPin2: "0000",
    newPin2Again: "1111",
    expectedError: {
      name: "emMmiError",
      additionalInformation: null
    }
  },
  
  {
    pin2: "0000",
    newPin2: "",
    newPin2Again: "",
    expectedError: {
      name: "emMmiError",
      additionalInformation: null
    }
  },
  
  {
    pin2: "0000",
    newPin2: "0000",
    newPin2Again: "1111",
    expectedError: {
      name: "emMmiErrorMismatchPin",
      additionalInformation: null
    }
  },
  
  {
    pin2: "123",
    newPin2: "0000",
    newPin2Again: "0000",
    expectedError: {
      name: "emMmiErrorInvalidPin",
      additionalInformation: null
    }
  },
  
  {
    pin2: "0000",
    newPin2: "123456789",
    newPin2Again: "123456789",
    expectedError: {
      name: "emMmiErrorInvalidPin",
      additionalInformation: null
    }
  },
  
  
  
  {
    pin2: "0000",
    newPin2: "0000",
    newPin2Again: "0000",
    expectedError: {
      name: "RequestNotSupported",
      additionalInformation: null
    }
  },
];

function testChangePin2(aPin2, aNewPin2, aNewPin2Again, aExpectedError) {
  let MMI_CODE = "**042*" + aPin2 + "*" + aNewPin2 + "*" + aNewPin2Again + "#";
  log("Test " + MMI_CODE);

  return gSendMMI(MMI_CODE).then(aResult => {
    is(aResult.success, !aExpectedError, "check success");
    is(aResult.serviceCode, "scPin2", "Check service code");

    if (aResult.success) {
      is(aResult.statusMessage, "smPin2Changed", "Check status message");
      is(aResult.additionalInformation, undefined, "Check additional information");
    } else {
      is(aResult.statusMessage, aExpectedError.name, "Check name");
      is(aResult.additionalInformation, aExpectedError.additionalInformation,
         "Check additional information");
    }
  });
}


startTest(function() {
  let promise = Promise.resolve();
  for (let i = 0; i < TEST_DATA.length; i++) {
    let data = TEST_DATA[i];
    promise = promise.then(() => testChangePin2(data.pin2,
                                                data.newPin2,
                                                data.newPin2Again,
                                                data.expectedError));
  }

  return promise
    .catch(error => ok(false, "Promise reject: " + error))
    .then(finish);
});
