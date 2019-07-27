


const {Cc: Cc, Ci: Ci, Cr: Cr, Cu: Cu} = SpecialPowers;

let Promise = Cu.import("resource://gre/modules/Promise.jsm").Promise;

const PDU_DCS_CODING_GROUP_BITS          = 0xF0;
const PDU_DCS_MSG_CODING_7BITS_ALPHABET  = 0x00;
const PDU_DCS_MSG_CODING_8BITS_ALPHABET  = 0x04;
const PDU_DCS_MSG_CODING_16BITS_ALPHABET = 0x08;

const PDU_DCS_MSG_CLASS_BITS             = 0x03;
const PDU_DCS_MSG_CLASS_NORMAL           = 0xFF;
const PDU_DCS_MSG_CLASS_0                = 0x00;
const PDU_DCS_MSG_CLASS_ME_SPECIFIC      = 0x01;
const PDU_DCS_MSG_CLASS_SIM_SPECIFIC     = 0x02;
const PDU_DCS_MSG_CLASS_TE_SPECIFIC      = 0x03;
const PDU_DCS_MSG_CLASS_USER_1           = 0x04;
const PDU_DCS_MSG_CLASS_USER_2           = 0x05;

const GECKO_SMS_MESSAGE_CLASSES = {};
GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]       = "normal";
GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_0]            = "class-0";
GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_ME_SPECIFIC]  = "class-1";
GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_SIM_SPECIFIC] = "class-2";
GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_TE_SPECIFIC]  = "class-3";
GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_USER_1]       = "user-1";
GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_USER_2]       = "user-2";

const CB_MESSAGE_SIZE_GSM  = 88;
const CB_MESSAGE_SIZE_ETWS = 56;

const CB_GSM_MESSAGEID_ETWS_BEGIN = 0x1100;
const CB_GSM_MESSAGEID_ETWS_END   = 0x1107;

const CB_GSM_GEOGRAPHICAL_SCOPE_NAMES = [
  "cell-immediate",
  "plmn",
  "location-area",
  "cell"
];

const CB_ETWS_WARNING_TYPE_NAMES = [
  "earthquake",
  "tsunami",
  "earthquake-tsunami",
  "test",
  "other"
];

const CB_DCS_LANG_GROUP_1 = [
  "de", "en", "it", "fr", "es", "nl", "sv", "da", "pt", "fi",
  "no", "el", "tr", "hu", "pl", null
];
const CB_DCS_LANG_GROUP_2 = [
  "cs", "he", "ar", "ru", "is", null, null, null, null, null,
  null, null, null, null, null, null
];

const CB_MAX_CONTENT_PER_PAGE_7BIT = Math.floor((CB_MESSAGE_SIZE_GSM - 6) * 8 / 7);
const CB_MAX_CONTENT_PER_PAGE_UCS2 = Math.floor((CB_MESSAGE_SIZE_GSM - 6) / 2);

const DUMMY_BODY_7BITS = "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
                       + "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
                       + "@@@@@@@@@@@@@"; 
const DUMMY_BODY_7BITS_IND = DUMMY_BODY_7BITS.substr(3);
const DUMMY_BODY_UCS2 = "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000"
                      + "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000"
                      + "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000"
                      + "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000"
                      + "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000"
                      + "\u0000"; 
const DUMMY_BODY_UCS2_IND = DUMMY_BODY_UCS2.substr(1);

is(DUMMY_BODY_7BITS.length,     CB_MAX_CONTENT_PER_PAGE_7BIT,     "DUMMY_BODY_7BITS.length");
is(DUMMY_BODY_7BITS_IND.length, CB_MAX_CONTENT_PER_PAGE_7BIT - 3, "DUMMY_BODY_7BITS_IND.length");
is(DUMMY_BODY_UCS2.length,      CB_MAX_CONTENT_PER_PAGE_UCS2,     "DUMMY_BODY_UCS2.length");
is(DUMMY_BODY_UCS2_IND.length,  CB_MAX_CONTENT_PER_PAGE_UCS2 - 1, "DUMMY_BODY_UCS2_IND.length");











function buildHexStr(aNum, aNumSemiOctets) {
  let str = aNum.toString(16);
  ok(str.length <= aNumSemiOctets);
  while (str.length < aNumSemiOctets) {
    str = "0" + str;
  }
  return str;
}











function decodeGsmDataCodingScheme(aDcs) {
  let language = null;
  let hasLanguageIndicator = false;
  let encoding = PDU_DCS_MSG_CODING_7BITS_ALPHABET;
  let messageClass = PDU_DCS_MSG_CLASS_NORMAL;

  switch (aDcs & PDU_DCS_CODING_GROUP_BITS) {
    case 0x00: 
      language = CB_DCS_LANG_GROUP_1[aDcs & 0x0F];
      break;

    case 0x10: 
      switch (aDcs & 0x0F) {
        case 0x00:
          hasLanguageIndicator = true;
          break;
        case 0x01:
          encoding = PDU_DCS_MSG_CODING_16BITS_ALPHABET;
          hasLanguageIndicator = true;
          break;
      }
      break;

    case 0x20: 
      language = CB_DCS_LANG_GROUP_2[aDcs & 0x0F];
      break;

    case 0x40: 
    case 0x50:
    
    
    case 0x90: 
      encoding = (aDcs & 0x0C);
      if (encoding == 0x0C) {
        encoding = PDU_DCS_MSG_CODING_7BITS_ALPHABET;
      }
      messageClass = (aDcs & PDU_DCS_MSG_CLASS_BITS);
      break;

    case 0xF0:
      encoding = (aDcs & 0x04) ? PDU_DCS_MSG_CODING_8BITS_ALPHABET
                              : PDU_DCS_MSG_CODING_7BITS_ALPHABET;
      switch(aDcs & PDU_DCS_MSG_CLASS_BITS) {
        case 0x01: messageClass = PDU_DCS_MSG_CLASS_USER_1; break;
        case 0x02: messageClass = PDU_DCS_MSG_CLASS_USER_2; break;
        case 0x03: messageClass = PDU_DCS_MSG_CLASS_TE_SPECIFIC; break;
      }
      break;

    case 0x30: 
    case 0x80: 
    case 0xA0: 
    case 0xB0:
    case 0xC0:
      break;
    default:
      throw new Error("Unsupported CBS data coding scheme: " + aDcs);
  }

  return [encoding, language, hasLanguageIndicator,
          GECKO_SMS_MESSAGE_CLASSES[messageClass]];
}












let cbManager;
function ensureCellBroadcast() {
  let deferred = Promise.defer();

  let permissions = [{
    "type": "cellbroadcast",
    "allow": 1,
    "context": document,
  }];
  SpecialPowers.pushPermissions(permissions, function() {
    ok(true, "permissions pushed: " + JSON.stringify(permissions));

    cbManager = window.navigator.mozCellBroadcast;
    if (cbManager) {
      log("navigator.mozCellBroadcast is instance of " + cbManager.constructor);
    } else {
      log("navigator.mozCellBroadcast is undefined.");
    }

    if (cbManager instanceof window.MozCellBroadcast) {
      deferred.resolve(cbManager);
    } else {
      deferred.reject();
    }
  });

  return deferred.promise;
}
















let pendingEmulatorCmdCount = 0;
function runEmulatorCmdSafe(aCommand) {
  let deferred = Promise.defer();

  ++pendingEmulatorCmdCount;
  runEmulatorCmd(aCommand, function(aResult) {
    --pendingEmulatorCmdCount;

    ok(true, "Emulator response: " + JSON.stringify(aResult));
    if (Array.isArray(aResult) && aResult[aResult.length - 1] === "OK") {
      deferred.resolve(aResult);
    } else {
      deferred.reject(aResult);
    }
  });

  return deferred.promise;
}















function sendRawCbsToEmulator(aPdu) {
  let command = "cbs pdu " + aPdu;
  return runEmulatorCmdSafe(command);
}













function waitForManagerEvent(aEventName) {
  let deferred = Promise.defer();

  cbManager.addEventListener(aEventName, function onevent(aEvent) {
    cbManager.removeEventListener(aEventName, onevent);

    ok(true, "Cellbroadcast event '" + aEventName + "' got.");
    deferred.resolve(aEvent);
  });

  return deferred.promise;
}


















function sendMultipleRawCbsToEmulatorAndWait(aPdus) {
  let promises = [];

  promises.push(waitForManagerEvent("received"));
  for (let pdu of aPdus) {
    promises.push(sendRawCbsToEmulator(pdu));
  }

  return Promise.all(promises).then(aResults => aResults[0].message);
}




function cleanUp() {
  
  ok(true, ":: CLEANING UP ::");

  waitFor(finish, function() {
    return pendingEmulatorCmdCount === 0;
  });
}















function selectModem(aServiceId) {
  let command = "mux modem " + aServiceId;
  return runEmulatorCmdSafe(command);
}








function runIfMultiSIM(aTest) {
  let numRIL;
  try {
    numRIL = SpecialPowers.getIntPref("ril.numRadioInterfaces");
  } catch (ex) {
    numRIL = 1;  
  }

  if (numRIL > 1) {
    return aTest();
  } else {
    log("Not a Multi-SIM environment. Test is skipped.");
    return Promise.resolve();
  }
}










function startTestCommon(aTestCaseMain) {
  Promise.resolve()
         .then(ensureCellBroadcast)
         .then(aTestCaseMain)
         .then(cleanUp, function() {
           ok(false, 'promise rejects during test.');
           cleanUp();
         });
}
