


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const PDU_SMSC_NONE = "00"; 



const PDU_FIRST_OCTET = "40";



const PDU_OA = "0AA89021436587";  

const PDU_PID_NORMAL = "00";
const PDU_DCS_GSM_7BIT = "00";
const PDU_TIMESTAMP = "51302151740020"; 







const PDU_UD_GSM = "C76913";

const IE_USE_SPANISH_LOCKING_SHIFT_TABLE = "250102";
const IE_USE_SPANISH_SINGLE_SHIFT_TABLE = "240102";
const PDU_UDHL = "06";

const PDU_UDL = "0B"; 

const PDU = PDU_SMSC_NONE + PDU_FIRST_OCTET + PDU_OA + PDU_PID_NORMAL
  + PDU_DCS_GSM_7BIT + PDU_TIMESTAMP + PDU_UDL + PDU_UDHL
  + IE_USE_SPANISH_LOCKING_SHIFT_TABLE + IE_USE_SPANISH_SINGLE_SHIFT_TABLE
  + PDU_UD_GSM;

function verifyMessage(aMessage) {
  is(aMessage.body, "GSM", "SmsMessage body");
}





startTestCommon(function testCaseMain() {
  return Promise.resolve()
    .then(() => sendMultipleRawSmsToEmulatorAndWait([PDU]))
    .then(results => verifyMessage(results[0].message));
});
