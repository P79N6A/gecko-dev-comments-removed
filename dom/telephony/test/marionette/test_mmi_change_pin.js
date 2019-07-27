


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

  return sendMMI(MMI_CODE)
    .then(function resolve(aResult) {
      ok(!aExpectedError, MMI_CODE + " success");
      is(aResult.serviceCode, "scPin", "Check service code");
      is(aResult.statusMessage, "smPinChanged", "Check status message");
      is(aResult.additionalInformation, undefined, "Check additional information");
    }, function reject(aError) {
      ok(aExpectedError, MMI_CODE + " fail");
      is(aError.name, aExpectedError.name, "Check name");
      is(aError.message, "", "Check message");
      is(aError.serviceCode, "scPin", "Check service code");
      is(aError.additionalInformation, aExpectedError.additionalInformation,
         "Check additional information");
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
  return promise.then(finish);
});
