


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  
  
  
  {
    reason: MozMobileConnection.CALL_FORWARD_REASON_ALL_CALL_FORWARDING,
    errorMsg: "GenericFailure"
  }, {
    reason: MozMobileConnection.CALL_FORWARD_REASON_ALL_CONDITIONAL_CALL_FORWARDING,
    errorMsg: "GenericFailure"
  },
  
  {
    reason: 10 ,
    errorMsg: "InvalidParameter"
  }
];

function testGetCallForwardingOptionError(aReason, aErrorMsg) {
  log("Test getting call forwarding for " + aReason);

  return getCallForwardingOption(aReason)
    .then(function resolve() {
      ok(false, "getCallForwardingOption success");
    }, function reject(aError) {
      is(aError.name, aErrorMsg, "failed to getCallForwardingOption");
    });
}


startTestCommon(function() {
  let promise = Promise.resolve();
  for (let i = 0; i < TEST_DATA.length; i++) {
    let data = TEST_DATA[i];
    promise = promise.then(() => testGetCallForwardingOptionError(data.reason,
                                                                  data.errorMsg));
  }
  return promise;
});
