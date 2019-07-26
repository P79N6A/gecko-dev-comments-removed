


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  
  
  
  
  {
    mode: "home",
    expectedErrorMessage: "RequestNotSupported"
  }, {
    mode: "affiliated",
    expectedErrorMessage: "RequestNotSupported"
  }, {
    mode: "any",
    expectedErrorMessage: "RequestNotSupported"
  },
  
  {
    mode: "InvalidMode",
    expectGotException: true
  }, {
    mode: null,
    expectGotException: true
  }
];

function testSetRoamingPreference(aMode, aExpectedErrorMsg, aExpectGotException) {
  log("Test setting roaming preference mode to " + aMode);

  try {
    return setRoamingPreference(aMode)
      .then(function resolve() {
        ok(!aExpectedErrorMsg, "setRoamingPreference success");
      }, function reject(aError) {
        is(aError.name, aExpectedErrorMsg, "failed to setRoamingPreference");
      });
  } catch (e) {
    ok(aExpectGotException, "caught an exception: " + e);
  }
}


startTestCommon(function() {
  let promise = Promise.resolve();
  for (let i = 0; i < TEST_DATA.length; i++) {
    let data = TEST_DATA[i];
    promise = promise.then(() => testSetRoamingPreference(data.mode,
                                                          data.expectedErrorMessage,
                                                          data.expectGotException));
  }
  return promise;
});
