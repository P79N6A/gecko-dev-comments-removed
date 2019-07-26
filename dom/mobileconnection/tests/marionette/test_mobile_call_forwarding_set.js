


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const TEST_NUMBER = "0912345678";
const TEST_TIME_SECONDS = 5;
const TEST_DATA = [
  
  
  
  
  {
    options: {
      action: MozMobileConnection.CALL_FORWARD_ACTION_DISABLE,
      reason: MozMobileConnection.CALL_FORWARD_REASON_UNCONDITIONAL,
    },
    expectedErrorMsg: "RequestNotSupported"
  }, {
    options: {
      action: MozMobileConnection.CALL_FORWARD_ACTION_ENABLE,
      reason: MozMobileConnection.CALL_FORWARD_REASON_MOBILE_BUSY,
    },
    expectedErrorMsg: "RequestNotSupported"
  },
  
  {
    options: {
      
      action: MozMobileConnection.CALL_FORWARD_ACTION_QUERY_STATUS,
      reason: MozMobileConnection.CALL_FORWARD_REASON_MOBILE_BUSY,
    },
    expectedErrorMsg: "InvalidParameter"
  }, {
    options: {
      action: 10 ,
      reason: MozMobileConnection.CALL_FORWARD_REASON_MOBILE_BUSY,
    },
    expectedErrorMsg: "InvalidParameter"
  },
  
  {
    options: {
      action: MozMobileConnection.CALL_FORWARD_ACTION_DISABLE,
      reason: 10 ,
    },
    expectedErrorMsg: "InvalidParameter"
  }
];

function testSetCallForwardingOption(aOptions, aExpectedErrorMsg) {
  log("Test setting call forwarding to " + JSON.stringify(aOptions));

  aOptions.number = TEST_NUMBER;
  aOptions.timeSeconds = TEST_TIME_SECONDS;

  return setCallForwardingOption(aOptions)
    .then(function resolve() {
      ok(!aExpectedErrorMsg, "setCallForwardingOption success");
    }, function reject(aError) {
      is(aError.name, aExpectedErrorMsg, "failed to setCallForwardingOption");
    });
}


startTestCommon(function() {
  let promise = Promise.resolve();
  for (let i = 0; i < TEST_DATA.length; i++) {
    let data = TEST_DATA[i];
    promise = promise.then(() => testSetCallForwardingOption(data.options,
                                                             data.expectedErrorMsg));
  }
  return promise;
});
