


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  
  {
    password: "",
    newPassword: "0000",
    newPasswordAgain: "1111",
    expectedError: {
      name: "emMmiErrorInvalidPassword"
    }
  },
  
  {
    password: "0000",
    newPassword: "",
    newPasswordAgain: "",
    expectedError: {
      name: "emMmiErrorInvalidPassword"
    }
  },
  
  {
    password: "0000",
    newPassword: "0000",
    newPasswordAgain: "1111",
    expectedError: {
      name: "emMmiErrorMismatchPassword"
    }
  },
  
  {
    password: "000",
    newPassword: "0000",
    newPasswordAgain: "0000",
    expectedError: {
      name: "emMmiErrorInvalidPassword"
    }
  },
  
  
  
  {
    password: "0000",
    newPassword: "1234",
    newPasswordAgain: "1234",
    expectedError: {
      name: "RequestNotSupported"
    }
  }
];

let MMI_PREFIX = [
  "*03*330*",
  "**03*330*",
  "*03**",
  "**03**",
];

function testChangeCallBarringPassword(aMMIPrefix, aPassword, aNewPassword,
                                       aNewPasswordAgain, aExpectedError) {
  let MMI_CODE = aMMIPrefix + aPassword + "*" + aNewPassword + "*" + aNewPasswordAgain + "#";
  log("Test " + MMI_CODE);

  return gSendMMI(MMI_CODE).then(aResult => {
    is(aResult.success, !aExpectedError, "check success");
    is(aResult.serviceCode, "scChangePassword", "Check service code");

    if (aResult.success) {
      is(aResult.statusMessage, "smPasswordChanged", "Check status message");
    } else {
      is(aResult.statusMessage, aExpectedError.name, "Check name");
    }
  });
}


startTest(function() {
  let promise = Promise.resolve();

  for (let prefix of MMI_PREFIX) {
    for (let i = 0; i < TEST_DATA.length; i++) {
      let data = TEST_DATA[i];
      promise = promise.then(() => testChangeCallBarringPassword(prefix,
                                                                 data.password,
                                                                 data.newPassword,
                                                                 data.newPasswordAgain,
                                                                 data.expectedError));
    }
  }

  return promise
    .catch(error => ok(false, "Promise reject: " + error))
    .then(finish);
});
