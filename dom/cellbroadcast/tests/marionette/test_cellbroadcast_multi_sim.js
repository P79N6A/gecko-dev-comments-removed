


MARIONETTE_TIMEOUT = 10000;
MARIONETTE_HEAD_JS = 'head.js';

const BODY_7BITS = "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
                 + "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
                 + "@@@@@@@@@@@@@"; 

function testReceiving_MultiSIM() {
  log("Test receiving GSM Cell Broadcast - Multi-SIM");

  let pdu = buildHexStr(0, CB_MESSAGE_SIZE_GSM * 2);

  let verifyCBMessage = (aMessage, aServiceId) => {
    log("Verify CB message received from serviceId: " + aServiceId);
    is(aMessage.body, BODY_7BITS, "Checking message body.");
    is(aMessage.serviceId, aServiceId, "Checking serviceId.");
  };

  return selectModem(1)
    .then(() => sendMultipleRawCbsToEmulatorAndWait([pdu]))
    .then((aMessage) => verifyCBMessage(aMessage, 1))
    .then(() => selectModem(0))
    .then(() => sendMultipleRawCbsToEmulatorAndWait([pdu]))
    .then((aMessage) => verifyCBMessage(aMessage, 0));
}

startTestCommon(function testCaseMain() {
  return runIfMultiSIM(testReceiving_MultiSIM);
});
