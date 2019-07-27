


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";


const PASSWORD = "0000";

const CB_TYPES = ["33", "331", "332", "35", "351"];
const CB_TYPES_UNSUPPORTED = ["330", "333", "353"];

const BS = "10";
const MMI_SERVICE_CLASS = ["serviceClassVoice", "serviceClassFax",
                           "serviceClassSms"];

const OPERATION_UNSUPPORTED = ["**", "##"];

function sendCbMMI(aOperation, aType, aExpectedSuccess, aExpectedStatusMessage) {
  let mmi = aOperation + aType + "*" + PASSWORD + "*" + BS + "#";
  log("Test " + mmi + " ...");

  return gSendMMI(mmi)
    .then((aResult) => {
      is(aResult.success, aExpectedSuccess, "Check success");
      is(aResult.serviceCode, "scCallBarring", "Check serviceCode");
      is(aResult.statusMessage, aExpectedStatusMessage,  "Check statusMessage");
      return aResult;
    });
}

function testCallBarring(aEnabled) {
  let promise = Promise.resolve();

  CB_TYPES.forEach(function(aType) {
    promise = promise
      
      .then(() => sendCbMMI(aEnabled ? "*" : "#", aType, true,
                            aEnabled ? "smServiceEnabled" : "smServiceDisabled"))
      
      .then(() => sendCbMMI("*#", aType, true,
                            aEnabled ? "smServiceEnabledFor": "smServiceDisabled"))
      .then(aResult => {
        if (aEnabled) {
          is(aResult.additionalInformation.length, MMI_SERVICE_CLASS.length,
             "Check additionalInformation.length");
          for (let i = 0; i < MMI_SERVICE_CLASS.length; i++) {
            is(aResult.additionalInformation[i], MMI_SERVICE_CLASS[i],
               "Check additionalInformation[" + i + "]");
          }
        }
      });
  });

  return promise;
}

function testUnsupportType() {
  let promise = Promise.resolve();

  CB_TYPES_UNSUPPORTED.forEach(function(aType) {
    promise = promise
      
      .then(() => sendCbMMI("*", aType, false, "RequestNotSupported"))
      
      .then(() => sendCbMMI("*#", aType, false, "RequestNotSupported"));
  });

  return promise;
}

function testUnsupportedOperation() {
  let promise = Promise.resolve();

  let types = CB_TYPES.concat(CB_TYPES_UNSUPPORTED);
  types.forEach(function(aType) {
    OPERATION_UNSUPPORTED.forEach(function(aOperation) {
      promise = promise
        .then(() => sendCbMMI(aOperation, aType, false,
                              "emMmiErrorNotSupported"));
    });
  });

  return promise;
}


startTest(function() {
  
  return testCallBarring(true)
    
    .then(() => testCallBarring(false))
    .then(() => testUnsupportType())
    .then(() => testUnsupportedOperation())

    .catch(error => ok(false, "Promise reject: " + error))
    .then(finish);;
});
