


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const number = "0900000001";
let outCall;

function getIMEI() {
  log("Test *#06# ...");

  return gSendMMI("*#06#").then(aResult => {
    ok(aResult.success, "success");
    is(aResult.serviceCode, "scImei", "Service code IMEI");
    
    
    
    is(aResult.statusMessage, "000000000000000", "Emulator IMEI");
    is(aResult.additionalInformation, undefined, "No additional information");
  });
}

function testInCallMMI_IMEI() {
  log('= testInCallMMI_IMEI =');

  return gDial(number)
    .then(call => outCall = call)
    .then(() => gRemoteAnswer(outCall))
    .then(() => getIMEI())
    .then(() => gRemoteHangUpCalls([outCall]));
}

startTest(function() {
  testInCallMMI_IMEI()
    .catch(error => ok(false, "Promise reject: " + error))
    .then(finish);
});
