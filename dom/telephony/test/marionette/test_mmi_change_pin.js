


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";




const TEST_DATA = [
  
  {
    pin: "",
    newPin: "0000",
    newPinAgain: "1111",
    expectedError: {
      name: "emMmiError",
      additionalInformation: null
    }
  },
  
  {
    pin: "0000",
    newPin: "",
    newPinAgain: "",
    expectedError: {
      name: "emMmiError",
      additionalInformation: null
    }
  },
  
  {
    pin: "0000",
    newPin: "0000",
    newPinAgain: "1111",
    expectedError: {
      name: "emMmiErrorMismatchPin",
      additionalInformation: null
    }
  },
  
  {
    pin: "000",
    newPin: "0000",
    newPinAgain: "0000",
    expectedError: {
      name: "emMmiErrorInvalidPin",
      additionalInformation: null
    }
  },
  
  {
    pin: "0000",
    newPin: "000000000",
    newPinAgain: "000000000",
    expectedError: {
      name: "emMmiErrorInvalidPin",
      additionalInformation: null
    }
  },
  
  {
    pin: "1234",
    newPin: "0000",
    newPinAgain: "0000",
    expectedError: {
      name: "emMmiErrorBadPin",
      
      additionalInformation: 2
    }
  },
  
  {
    pin: "0000",
    newPin: "0000",
    newPinAgain: "0000"
  }
];

function testChangePin(aPin, aNewPin, aNewPinAgain, aExpectedError) {
  let MMI_CODE = "**04*" + aPin + "*" + aNewPin + "*" + aNewPinAgain + "#";
  log("Test " + MMI_CODE);

  return gSendMMI(MMI_CODE).then(aResult => {
    is(aResult.success, !aExpectedError, "check success");
    is(aResult.serviceCode, "scPin", "Check service code");

    if (aResult.success) {
      is(aResult.statusMessage, "smPinChanged", "Check status message");
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
    promise = promise.then(() => testChangePin(data.pin,
                                               data.newPin,
                                               data.newPinAgain,
                                               data.expectedError));
  }

  return promise
    .then(null, cause => {
      ok(false, 'promise rejects during test: ' + cause);
    })
    .then(finish);
});
