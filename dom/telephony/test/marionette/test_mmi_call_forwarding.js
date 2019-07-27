


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

let connection;













function waitForManagerEvent(aEventName) {
  let deferred = Promise.defer();

  connection.addEventListener(aEventName, function onevent(aEvent) {
    connection.removeEventListener(aEventName, onevent);

    ok(true, "MobileConnection event '" + aEventName + "' got.");
    deferred.resolve(aEvent);
  });

  return deferred.promise;
}












function wrapDomRequestAsPromise(aRequest) {
  let deferred = Promise.defer();

  ok(aRequest instanceof DOMRequest,
     "aRequest is instanceof " + aRequest.constructor);

  aRequest.addEventListener("success", function(aEvent) {
    deferred.resolve(aEvent);
  });
  aRequest.addEventListener("error", function(aEvent) {
    deferred.reject(aEvent);
  });

  return deferred.promise;
}














function setCallForwardingOption(aOptions) {
  let request = connection.setCallForwardingOption(aOptions);
  return wrapDomRequestAsPromise(request)
    .then(null, () => { throw request.error; });
}

const TEST_DATA = [
  {
    reason: MozMobileConnection.CALL_FORWARD_REASON_UNCONDITIONAL,
    number: "+886912345678",
    serviceClass: MozMobileConnection.ICC_SERVICE_CLASS_VOICE,
    timeSeconds: 5
  }, {
    reason: MozMobileConnection.CALL_FORWARD_REASON_MOBILE_BUSY,
    number: "0912345678",
    serviceClass: MozMobileConnection.ICC_SERVICE_CLASS_VOICE,
    timeSeconds: 10
  }, {
    reason: MozMobileConnection.CALL_FORWARD_REASON_NO_REPLY,
    number: "+886987654321",
    serviceClass: MozMobileConnection.ICC_SERVICE_CLASS_VOICE,
    timeSeconds: 15
  }, {
    reason: MozMobileConnection.CALL_FORWARD_REASON_NOT_REACHABLE,
    number: "+0987654321",
    serviceClass: MozMobileConnection.ICC_SERVICE_CLASS_VOICE,
    timeSeconds: 20
  }
];


const CF_REASON_TO_MMI = {
  
  0: "21",
  
  1: "67",
  
  2: "61",
  
  3: "62",
  
  4: "002",
  
  5: "004"
};


const SERVICE_CLASS_TO_MMI = {
  
  1: "11"
};

function testSetCallForwarding(aData) {
  
  let MMI_CODE = "**" + CF_REASON_TO_MMI[aData.reason] + "*" + aData.number +
                 "*" + SERVICE_CLASS_TO_MMI[aData.serviceClass] +
                 "*" + aData.timeSeconds + "#";
  log("Test " + MMI_CODE);

  let promises = [];
  
  promises.push(waitForManagerEvent("cfstatechange").then(function(aEvent) {
    is(aEvent.success, true, "check success");
    is(aEvent.action, MozMobileConnection.CALL_FORWARD_ACTION_REGISTRATION,
       "check action");
    is(aEvent.reason, aData.reason, "check reason");
    is(aEvent.number, aData.number, "check number");
    is(aEvent.timeSeconds, aData.timeSeconds, "check timeSeconds");
    is(aEvent.serviceClass, aData.serviceClass, "check serviceClass");
  }));
  
  promises.push(sendMMI(MMI_CODE)
    .then(function resolve(aResult) {
      is(aResult.serviceCode, "scCallForwarding", "Check service code");
      is(aResult.statusMessage, "smServiceRegistered", "Check status message");
      is(aResult.additionalInformation, undefined, "Check additional information");
    }, function reject(aError) {
      ok(false, "got '" + aError.name + "' error");
    }));

  return Promise.all(promises);
}

function testGetCallForwarding(aExpectedData) {
  
  let MMI_CODE = "*#" + CF_REASON_TO_MMI[aExpectedData.reason] + "#";
  log("Test " + MMI_CODE);

  return sendMMI(MMI_CODE)
    .then(function resolve(aResult) {
      is(aResult.serviceCode, "scCallForwarding", "Check service code");
      is(aResult.statusMessage, "smServiceInterrogated", "Check status message");
      is(Array.isArray(aResult.additionalInformation), true,
         "additionalInformation should be an array");

      for (let i = 0; i < aResult.additionalInformation.length; i++) {
        let result = aResult.additionalInformation[i];

        
        
        if (!(result.serviceClass & aExpectedData.serviceClass)) {
          continue;
        }

        is(result.active, true, "check active");
        is(result.reason, aExpectedData.reason, "check reason");
        is(result.number, aExpectedData.number, "check number");
        is(result.timeSeconds, aExpectedData.timeSeconds, "check timeSeconds");
      }
    }, function reject(aError) {
      ok(false, MMI_CODE + " got error: " + aError.name);
    });
}

function clearAllCallForwardingSettings() {
  log("Clear all call forwarding settings");

  let promise = Promise.resolve();
  for (let reason = MozMobileConnection.CALL_FORWARD_REASON_UNCONDITIONAL;
       reason <= MozMobileConnection.CALL_FORWARD_REASON_ALL_CONDITIONAL_CALL_FORWARDING;
       reason++) {
    let options = {
      reason: reason,
      action: MozMobileConnection.CALL_FORWARD_ACTION_ERASURE
    };
    
    
    promise =
      promise.then(() => setCallForwardingOption(options).then(null, () => {}));
  }
  return promise;
}


startTestWithPermissions(['mobileconnection'], function() {
  connection = navigator.mozMobileConnections[0];

  let promise = Promise.resolve();
  for (let i = 0; i < TEST_DATA.length; i++) {
    let data = TEST_DATA[i];
    promise = promise.then(() => testSetCallForwarding(data))
                     .then(() => testGetCallForwarding(data));
  }
  
  return promise.then(null, () => { ok(false, "promise reject during test"); })
    .then(() => clearAllCallForwardingSettings())
    .then(finish);
});
