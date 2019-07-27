MARIONETTE_TIMEOUT = 90000;
MARIONETTE_HEAD_JS = 'head.js';

const CB_UMTS_MESSAGE_TYPE_CBS = 1;
const CB_UMTS_MESSAGE_PAGE_SIZE = 82;

function testReceiving_UMTS_MessageAttributes() {
  log("Test receiving UMTS Cell Broadcast - Message Attributes");

  let verifyCBMessage = (aMessage) => {
    
    ok(aMessage.gsmGeographicalScope != null, "aMessage.gsmGeographicalScope");
    ok(aMessage.messageCode != null, "aMessage.messageCode");
    ok(aMessage.messageId != null, "aMessage.messageId");
    ok(aMessage.messageClass != null, "aMessage.messageClass");
    ok(aMessage.timestamp != null, "aMessage.timestamp");
    ok('etws' in aMessage, "aMessage.etws");
    if (aMessage.etws) {
      ok('warningType' in aMessage.etws, "aMessage.etws.warningType");
      ok(aMessage.etws.emergencyUserAlert != null, "aMessage.etws.emergencyUserAlert");
      ok(aMessage.etws.popup != null, "aMessage.etws.popup");
    }
    ok(aMessage.cdmaServiceCategory != null, "aMessage.cdmaServiceCategory");
  };

  
  let pdu = buildHexStr(CB_UMTS_MESSAGE_TYPE_CBS, 2) 
          + buildHexStr(0, 10) 
          + buildHexStr(1, 2)  
          + buildHexStr(0, CB_UMTS_MESSAGE_PAGE_SIZE * 2)
          + buildHexStr(CB_UMTS_MESSAGE_PAGE_SIZE, 2); 

  return sendMultipleRawCbsToEmulatorAndWait([pdu])
    .then((aMessage) => verifyCBMessage(aMessage));
}

function testReceiving_UMTS_GeographicalScope() {
  log("Test receiving UMTS Cell Broadcast - Geographical Scope");

  let promise = Promise.resolve();

  let verifyCBMessage = (aMessage, aGsName) => {
    is(aMessage.gsmGeographicalScope, aGsName,
       "aMessage.gsmGeographicalScope");
  };

  CB_GSM_GEOGRAPHICAL_SCOPE_NAMES.forEach(function(aGsName, aIndex) {
    let pdu = buildHexStr(CB_UMTS_MESSAGE_TYPE_CBS, 2) 
            + buildHexStr(0, 4) 
            + buildHexStr(((aIndex & 0x03) << 14), 4) 
            + buildHexStr(0, 2) 
            + buildHexStr(1, 2) 
            + buildHexStr(0, CB_UMTS_MESSAGE_PAGE_SIZE * 2)
            + buildHexStr(CB_UMTS_MESSAGE_PAGE_SIZE, 2); 
    promise = promise
      .then(() => sendMultipleRawCbsToEmulatorAndWait([pdu]))
      .then((aMessage) => verifyCBMessage(aMessage, aGsName));
  });

  return promise;
}

function testReceiving_UMTS_MessageCode() {
  log("Test receiving UMTS Cell Broadcast - Message Code");

  let promise = Promise.resolve();

  
  
  let messageCodes = [
    0x000, 0x001, 0x002, 0x004, 0x008, 0x010, 0x020, 0x040,
    0x080, 0x100, 0x200, 0x251
  ];

  let verifyCBMessage = (aMessage, aMsgCode) => {
    is(aMessage.messageCode, aMsgCode, "aMessage.messageCode");
  };

  messageCodes.forEach(function(aMsgCode) {
    let pdu = buildHexStr(CB_UMTS_MESSAGE_TYPE_CBS, 2) 
            + buildHexStr(0, 4) 
            + buildHexStr(((aMsgCode & 0x3FF) << 4), 4) 
            + buildHexStr(0, 2) 
            + buildHexStr(1, 2) 
            + buildHexStr(0, CB_UMTS_MESSAGE_PAGE_SIZE * 2)
            + buildHexStr(CB_UMTS_MESSAGE_PAGE_SIZE, 2); 
    promise = promise
      .then(() => sendMultipleRawCbsToEmulatorAndWait([pdu]))
      .then((aMessage) => verifyCBMessage(aMessage, aMsgCode));
  });

  return promise;
}

function testReceiving_UMTS_MessageId() {
  log("Test receiving UMTS Cell Broadcast - Message Identifier");

  let promise = Promise.resolve();

  
  
  let messageIds = [
    0x0000, 0x0001, 0x0010, 0x0100, 0x1000, 0x1111, 0x8888, 0x8811,
  ];

  let verifyCBMessage = (aMessage, aMessageId) => {
    is(aMessage.messageId, aMessageId, "aMessage.messageId");
    ok(aMessage.etws == null, "aMessage.etws");
  };

  messageIds.forEach(function(aMessageId) {
    let pdu = buildHexStr(CB_UMTS_MESSAGE_TYPE_CBS, 2) 
            + buildHexStr((aMessageId & 0xFFFF), 4) 
            + buildHexStr(0, 4) 
            + buildHexStr(0, 2) 
            + buildHexStr(1, 2) 
            + buildHexStr(0, CB_UMTS_MESSAGE_PAGE_SIZE * 2)
            + buildHexStr(CB_UMTS_MESSAGE_PAGE_SIZE, 2);  
    promise = promise
      .then(() => sendMultipleRawCbsToEmulatorAndWait([pdu]))
      .then((aMessage) => verifyCBMessage(aMessage, aMessageId));
  });

  return promise;
}

function testReceiving_UMTS_Language_and_Body() {
  log("Test receiving UMTS Cell Broadcast - Language & Body");

  let promise = Promise.resolve();

  let testDcs = [];
  let dcs = 0;
  while (dcs <= 0xFF) {
    try {
      let dcsInfo = { dcs: dcs };
      [ dcsInfo.encoding, dcsInfo.language,
        dcsInfo.indicator, dcsInfo.messageClass ] = decodeGsmDataCodingScheme(dcs);
      testDcs.push(dcsInfo);
    } catch (e) {
      
      dcs = (dcs & PDU_DCS_CODING_GROUP_BITS) + 0x10;
    }
    dcs++;
  }

  let verifyCBMessage = (aMessage, aDcsInfo) => {
    if (aDcsInfo.language) {
      is(aMessage.language, aDcsInfo.language, "aMessage.language");
    } else if (aDcsInfo.indicator) {
      is(aMessage.language, "@@", "aMessage.language");
    } else {
      ok(aMessage.language == null, "aMessage.language");
    }

    switch (aDcsInfo.encoding) {
      case PDU_DCS_MSG_CODING_7BITS_ALPHABET:
        is(aMessage.body,
           aDcsInfo.indicator ? DUMMY_BODY_7BITS_IND : DUMMY_BODY_7BITS,
           "aMessage.body");
        break;
      case PDU_DCS_MSG_CODING_8BITS_ALPHABET:
        ok(aMessage.body == null, "aMessage.body");
        break;
      case PDU_DCS_MSG_CODING_16BITS_ALPHABET:
        is(aMessage.body,
           aDcsInfo.indicator ? DUMMY_BODY_UCS2_IND : DUMMY_BODY_UCS2,
           "aMessage.body");
        break;
    }

    is(aMessage.messageClass, aDcsInfo.messageClass, "aMessage.messageClass");
  };

  testDcs.forEach(function(aDcsInfo) {
    let pdu = buildHexStr(CB_UMTS_MESSAGE_TYPE_CBS, 2) 
            + buildHexStr(0, 4) 
            + buildHexStr(0, 4) 
            + buildHexStr(aDcsInfo.dcs, 2) 
            + buildHexStr(1, 2) 
            + buildHexStr(0, CB_UMTS_MESSAGE_PAGE_SIZE * 2)
            + buildHexStr(CB_UMTS_MESSAGE_PAGE_SIZE, 2);  
    promise = promise
      .then(() => sendMultipleRawCbsToEmulatorAndWait([pdu]))
      .then((aMessage) => verifyCBMessage(aMessage, aDcsInfo));
  });

  return promise;
}

function testReceiving_UMTS_Timestamp() {
  log("Test receiving UMTS Cell Broadcast - Timestamp");

  let verifyCBMessage = (aMessage) => {
    
    
    let msMessage = aMessage.timestamp;
    let msNow = Date.now();
    ok(Math.abs(msMessage - msNow) < (1000 * 60), "aMessage.timestamp");
  };

  
  let pdu = buildHexStr(CB_UMTS_MESSAGE_TYPE_CBS, 2) 
          + buildHexStr(0, 10) 
          + buildHexStr(1, 2)  
          + buildHexStr(0, CB_UMTS_MESSAGE_PAGE_SIZE * 2)
          + buildHexStr(CB_UMTS_MESSAGE_PAGE_SIZE, 2); 

  return sendMultipleRawCbsToEmulatorAndWait([pdu])
    .then((aMessage) => verifyCBMessage(aMessage));
}

function testReceiving_UMTS_WarningType() {
  log("Test receiving UMTS Cell Broadcast - Warning Type");

  let promise = Promise.resolve();

  let messageIds = [];
  for (let i = CB_GSM_MESSAGEID_ETWS_BEGIN; i <= CB_GSM_MESSAGEID_ETWS_END; i++) {
    messageIds.push(i);
  }

  let verifyCBMessage = (aMessage, aMessageId) => {
    is(aMessage.messageId, aMessageId, "aMessage.messageId");
    ok(aMessage.etws != null, "aMessage.etws");

    let offset = aMessageId - CB_GSM_MESSAGEID_ETWS_BEGIN;
    if (offset < CB_ETWS_WARNING_TYPE_NAMES.length) {
      is(aMessage.etws.warningType, CB_ETWS_WARNING_TYPE_NAMES[offset],
         "aMessage.etws.warningType");
    } else {
      ok(aMessage.etws.warningType == null, "aMessage.etws.warningType");
    }
  };

  messageIds.forEach(function(aMessageId) {
    let pdu = buildHexStr(CB_UMTS_MESSAGE_TYPE_CBS, 2) 
            + buildHexStr((aMessageId & 0xFFFF), 4) 
            + buildHexStr(0, 4) 
            + buildHexStr(0, 2) 
            + buildHexStr(1, 2) 
            + buildHexStr(0, CB_UMTS_MESSAGE_PAGE_SIZE * 2)
            + buildHexStr(CB_UMTS_MESSAGE_PAGE_SIZE, 2); 
    promise = promise
      .then(() => sendMultipleRawCbsToEmulatorAndWait([pdu]))
      .then((aMessage) => verifyCBMessage(aMessage, aMessageId));
  });

  return promise;
}

function testReceiving_UMTS_EmergencyUserAlert() {
  log("Test receiving UMTS Cell Broadcast - Emergency User Alert");

  let promise = Promise.resolve();

  let emergencyUserAlertMasks = [0x2000, 0x0000];

  let verifyCBMessage = (aMessage, aMask) => {
    is(aMessage.messageId, CB_GSM_MESSAGEID_ETWS_BEGIN, "aMessage.messageId");
    ok(aMessage.etws != null, "aMessage.etws");
    is(aMessage.etws.emergencyUserAlert, aMask != 0, "aMessage.etws.emergencyUserAlert");
  };

  emergencyUserAlertMasks.forEach(function(aMask) {
    let pdu = buildHexStr(CB_UMTS_MESSAGE_TYPE_CBS, 2) 
            + buildHexStr(CB_GSM_MESSAGEID_ETWS_BEGIN, 4) 
            + buildHexStr(aMask, 4) 
            + buildHexStr(0, 2) 
            + buildHexStr(1, 2) 
            + buildHexStr(0, CB_UMTS_MESSAGE_PAGE_SIZE * 2)
            + buildHexStr(CB_UMTS_MESSAGE_PAGE_SIZE, 2); 
    promise = promise
      .then(() => sendMultipleRawCbsToEmulatorAndWait([pdu]))
      .then((aMessage) => verifyCBMessage(aMessage, aMask));
  });

  return promise;
}

function testReceiving_UMTS_Popup() {
  log("Test receiving UMTS Cell Broadcast - Popup");

  let promise = Promise.resolve();

  let popupMasks = [0x1000, 0x0000];

  let verifyCBMessage = (aMessage, aMask) => {
    is(aMessage.messageId, CB_GSM_MESSAGEID_ETWS_BEGIN, "aMessage.messageId");
    ok(aMessage.etws != null, "aMessage.etws");
    is(aMessage.etws.popup, aMask != 0, "aMessage.etws.popup");
  };

  popupMasks.forEach(function(aMask) {
    let pdu = buildHexStr(CB_UMTS_MESSAGE_TYPE_CBS, 2) 
            + buildHexStr(CB_GSM_MESSAGEID_ETWS_BEGIN, 4) 
            + buildHexStr(aMask, 4) 
            + buildHexStr(0, 2) 
            + buildHexStr(1, 2) 
            + buildHexStr(0, CB_UMTS_MESSAGE_PAGE_SIZE * 2)
            + buildHexStr(CB_UMTS_MESSAGE_PAGE_SIZE, 2); 
    promise = promise
      .then(() => sendMultipleRawCbsToEmulatorAndWait([pdu]))
      .then((aMessage) => verifyCBMessage(aMessage, aMask));
  });

  return promise;
}

function testReceiving_UMTS_Multipart() {
  log("Test receiving UMTS Cell Broadcast - Multipart Messages");

  let promise = Promise.resolve();

  
  
  
  
  
  let numOfPages = [1, 2, 3, 4, 5, 6];

  let verifyCBMessage = (aMessage, aNumOfPages) => {
      is(aMessage.body.length, (aNumOfPages * CB_MAX_CONTENT_PER_PAGE_7BIT),
         "aMessage.body");
  };

  numOfPages.forEach(function(aNumOfPages) {
    let pdu = buildHexStr(CB_UMTS_MESSAGE_TYPE_CBS, 2) 
            + buildHexStr(0, 4) 
            + buildHexStr(0, 4) 
            + buildHexStr(0, 2) 
            + buildHexStr(aNumOfPages, 2); 
    for (let i = 1; i <= aNumOfPages; i++) {
      pdu = pdu + buildHexStr(0, CB_UMTS_MESSAGE_PAGE_SIZE * 2)
                + buildHexStr(CB_UMTS_MESSAGE_PAGE_SIZE, 2); 
    }
    promise = promise
      .then(() => sendMultipleRawCbsToEmulatorAndWait([pdu]))
      .then((aMessage) => verifyCBMessage(aMessage, aNumOfPages));
  });

  return promise;
}

function testReceiving_UMTS_PaddingCharacters() {
  log("Test receiving UMTS Cell Broadcast - Padding Characters <CR>");

  let promise = Promise.resolve();

  let testContents = [
    { pdu:
        
        
        
        
        
        buildHexStr(CB_UMTS_MESSAGE_TYPE_CBS, 2) + 
        buildHexStr(0, 4) + 
        buildHexStr(0, 4) + 
        buildHexStr(0, 2) + 
        buildHexStr(1, 2) + 
        "54741914AFA7C76B9058FEBEBB41E637" +
        "1EA4AEB7E173D0DB5E9683E8E832881D" +
        "D6E741E4F7B9D168341A8D46A3D16834" +
        "1A8D46A3D168341A8D46A3D168341A8D" +
        "46A3D168341A8D46A3D168341A8D46A3" +
        "D100" +
        buildHexStr(CB_UMTS_MESSAGE_PAGE_SIZE, 2),  
      text:
        "The quick brown fox jumps over the lazy dog"
    },
    { pdu:
        
        
        buildHexStr(CB_UMTS_MESSAGE_TYPE_CBS, 2) + 
        buildHexStr(0, 4) + 
        buildHexStr(0, 4) + 
        buildHexStr(72, 2) + 
        buildHexStr(1, 2) + 
        "00540068006500200071007500690063" +
        "006b002000620072006f0077006e0020" +
        "0066006f00780020006a0075006d0070" +
        "00730020006f007600650072000D000D" +
        "000D000D000D000D000D000D000D000D" +
        "000D" +
        buildHexStr(CB_UMTS_MESSAGE_PAGE_SIZE, 2),  
      text:
        "The quick brown fox jumps over"
    }
  ];

  let verifyCBMessage = (aMessage, aText) => {
    is(aMessage.body, aText, "aMessage.body");
  };

  testContents.forEach(function(aTestContent) {
    promise = promise
      .then(() => sendMultipleRawCbsToEmulatorAndWait([aTestContent.pdu]))
      .then((aMessage) => verifyCBMessage(aMessage, aTestContent.text));
  });

  return promise;
}

function testReceiving_UMTS_MessageInformationLength() {
  log("Test receiving UMTS Cell Broadcast - Message Information Length");

  let testText = "The quick brown fox jumps over the lazy dog";

  let verifyCBMessage = (aMessage) => {
    is(aMessage.body, testText, "aMessage.body");
  };

  
  
  
  
  
  
  let pdu = buildHexStr(CB_UMTS_MESSAGE_TYPE_CBS, 2) 
          + buildHexStr(0, 4) 
          + buildHexStr(0, 4) 
          + buildHexStr(0, 2) 
          + buildHexStr(1, 2) 
          + "54741914AFA7C76B9058FEBEBB41E637"
          + "1EA4AEB7E173D0DB5E9683E8E832881D"
          + "D6E741E4F7B9D168341A8D46A3D16834"
          + "1A8D46A3D168341A8D46A3D168341A8D"
          + "46A3D168341A8D46A3D168341A8D46A3"
          + "D100"
          + buildHexStr(Math.ceil(testText.length * 7 / 8), 2); 

  return sendMultipleRawCbsToEmulatorAndWait([pdu])
    .then((aMessage) => verifyCBMessage(aMessage));
}

startTestCommon(function testCaseMain() {
  return testReceiving_UMTS_MessageAttributes()
  .then(() => testReceiving_UMTS_GeographicalScope())
  .then(() => testReceiving_UMTS_MessageCode())
  .then(() => testReceiving_UMTS_MessageId())
  .then(() => testReceiving_UMTS_Language_and_Body())
  .then(() => testReceiving_UMTS_Timestamp())
  .then(() => testReceiving_UMTS_WarningType())
  .then(() => testReceiving_UMTS_EmergencyUserAlert())
  .then(() => testReceiving_UMTS_Popup())
  .then(() => testReceiving_UMTS_Multipart())
  .then(() => testReceiving_UMTS_PaddingCharacters())
  .then(() => testReceiving_UMTS_MessageInformationLength());
});
