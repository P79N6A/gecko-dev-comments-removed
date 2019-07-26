


subscriptLoader.loadSubScript("resource://gre/modules/ril_consts.js", this);

const ESCAPE = "\uffff";
const RESCTL = "\ufffe";
const LF = "\n";
const CR = "\r";
const SP = " ";
const FF = "\u000c";

function run_test() {
  run_next_test();
}




add_test(function test_nl_locking_shift_tables_validity() {
  for (let lst = 0; lst < PDU_NL_LOCKING_SHIFT_TABLES.length; lst++) {
    do_print("Verifying PDU_NL_LOCKING_SHIFT_TABLES[" + lst + "]");

    let table = PDU_NL_LOCKING_SHIFT_TABLES[lst];

    
    do_check_eq(table.length, 128);

    
    do_check_eq(table[PDU_NL_EXTENDED_ESCAPE], ESCAPE);
    do_check_eq(table[PDU_NL_LINE_FEED], LF);
    do_check_eq(table[PDU_NL_CARRIAGE_RETURN], CR);
    do_check_eq(table[PDU_NL_SPACE], SP);
  }

  run_next_test();
});

add_test(function test_nl_single_shift_tables_validity() {
  for (let sst = 0; sst < PDU_NL_SINGLE_SHIFT_TABLES.length; sst++) {
    do_print("Verifying PDU_NL_SINGLE_SHIFT_TABLES[" + sst + "]");

    let table = PDU_NL_SINGLE_SHIFT_TABLES[sst];

    
    do_check_eq(table.length, 128);

    
    do_check_eq(table[PDU_NL_EXTENDED_ESCAPE], ESCAPE);
    do_check_eq(table[PDU_NL_PAGE_BREAK], FF);
    do_check_eq(table[PDU_NL_RESERVED_CONTROL], RESCTL);
  }

  run_next_test();
});

add_test(function test_gsm_sms_strict_7bit_charmap_validity() {
  let defaultTable = PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
  for (let from in GSM_SMS_STRICT_7BIT_CHARMAP) {
    let to = GSM_SMS_STRICT_7BIT_CHARMAP[from];
    do_print("Verifying GSM_SMS_STRICT_7BIT_CHARMAP[\"\\u0x"
             + from.charCodeAt(0).toString(16) + "\"] => \"\\u"
             + to.charCodeAt(0).toString(16) + "\"");

    
    do_check_eq(defaultTable.indexOf(from), -1);
    
    do_check_eq(defaultTable.indexOf(to) >= 0, true);
  }

  run_next_test();
});




add_test(function test_GsmPDUHelper_readDataCodingScheme() {
  let worker = newWorker({
    postRILMessage: function fakePostRILMessage(data) {
      
    },
    postMessage: function fakePostMessage(message) {
      
    }
  });

  let helper = worker.GsmPDUHelper;
  function test_dcs(dcs, encoding, messageClass, mwi) {
    helper.readHexOctet = function () {
      return dcs;
    }

    let msg = {};
    helper.readDataCodingScheme(msg);

    do_check_eq(msg.dcs, dcs);
    do_check_eq(msg.encoding, encoding);
    do_check_eq(msg.messageClass, messageClass);
    do_check_eq(msg.mwi == null, mwi == null);
    if (mwi != null) {
      do_check_eq(msg.mwi.active, mwi.active);
      do_check_eq(msg.mwi.discard, mwi.discard);
      do_check_eq(msg.mwi.msgCount, mwi.msgCount);
    }
  }

  
  
  test_dcs(0x00, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);
  test_dcs(0x04, PDU_DCS_MSG_CODING_8BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);
  test_dcs(0x08, PDU_DCS_MSG_CODING_16BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);
  test_dcs(0x0C, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);
  
  
  test_dcs(0x01, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);
  test_dcs(0x02, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);
  test_dcs(0x03, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);
  
  test_dcs(0x10, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_0]);
  test_dcs(0x11, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_1]);
  test_dcs(0x12, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_2]);
  test_dcs(0x13, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_3]);

  
  test_dcs(0x50, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_0]);

  
  test_dcs(0x8F, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);
  test_dcs(0x9F, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);
  test_dcs(0xAF, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);
  test_dcs(0xBF, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]);

  
  
  test_dcs(0xC0, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL],
           {active: false, discard: true, msgCount: 0});
  test_dcs(0xC8, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL],
           {active: true, discard: true, msgCount: -1});
  
  test_dcs(0xCC, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL],
           {active: true, discard: true, msgCount: -1});

  
  
  test_dcs(0xD0, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL],
           {active: false, discard: false, msgCount: 0});
  test_dcs(0xD8, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL],
           {active: true, discard: false, msgCount: -1});
  
  test_dcs(0xDC, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL],
           {active: true, discard: false, msgCount: -1});

  
  
  test_dcs(0xE0, PDU_DCS_MSG_CODING_16BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL],
           {active: false, discard: false, msgCount: 0});
  test_dcs(0xE8, PDU_DCS_MSG_CODING_16BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL],
           {active: true, discard: false, msgCount: -1});
  
  test_dcs(0xEC, PDU_DCS_MSG_CODING_16BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL],
           {active: true, discard: false, msgCount: -1});

  
  test_dcs(0xF0, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_0]);
  test_dcs(0xF1, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_1]);
  test_dcs(0xF2, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_2]);
  test_dcs(0xF3, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_3]);
  test_dcs(0xF4, PDU_DCS_MSG_CODING_8BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_0]);
  test_dcs(0xF5, PDU_DCS_MSG_CODING_8BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_1]);
  test_dcs(0xF6, PDU_DCS_MSG_CODING_8BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_2]);
  test_dcs(0xF7, PDU_DCS_MSG_CODING_8BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_3]);
  
  
  test_dcs(0xF8, PDU_DCS_MSG_CODING_7BITS_ALPHABET,
           GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_0]);

  run_next_test();
});





add_test(function test_RadioInterfaceLayer__countGsm7BitSeptets() {
  let ril = newRadioInterfaceLayer();

  let worker = newWorker({
    postRILMessage: function fakePostRILMessage(data) {
      
    },
    postMessage: function fakePostMessage(message) {
      
    }
  });

  let helper = worker.GsmPDUHelper;
  helper.resetOctetWritten = function () {
    helper.octetsWritten = 0;
  };
  helper.writeHexOctet = function () {
    helper.octetsWritten++;
  };

  function do_check_calc(str, expectedCalcLen, lst, sst, strict7BitEncoding) {
    do_check_eq(expectedCalcLen,
                ril._countGsm7BitSeptets(str,
                                         PDU_NL_LOCKING_SHIFT_TABLES[lst],
                                         PDU_NL_SINGLE_SHIFT_TABLES[sst],
                                         strict7BitEncoding));

    helper.resetOctetWritten();
    helper.writeStringAsSeptets(str, 0, lst, sst, strict7BitEncoding);
    do_check_eq(Math.ceil(expectedCalcLen * 7 / 8), helper.octetsWritten);
  }

  
  for (let lst = 0; lst < PDU_NL_LOCKING_SHIFT_TABLES.length; lst++) {
    let langTable = PDU_NL_LOCKING_SHIFT_TABLES[lst];

    let str = langTable.substring(0, PDU_NL_EXTENDED_ESCAPE)
              + langTable.substring(PDU_NL_EXTENDED_ESCAPE + 1);

    for (let sst = 0; sst < PDU_NL_SINGLE_SHIFT_TABLES.length; sst++) {
      let langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[sst];

      
      do_check_calc(ESCAPE + RESCTL, 0, lst, sst);

      
      do_check_calc(str, str.length, lst, sst);

      let [str1, str2] = ["", ""];
      for (let i = 0; i < langShiftTable.length; i++) {
        if ((i == PDU_NL_EXTENDED_ESCAPE) || (i == PDU_NL_RESERVED_CONTROL)) {
          continue;
        }

        let c = langShiftTable[i];
        if (langTable.indexOf(c) >= 0) {
          str1 += c;
        } else {
          str2 += c;
        }
      }

      
      
      do_check_calc(str1, str1.length, lst, sst);

      
      
      do_check_calc(str2, str2.length * 2, lst, sst);
    }
  }

  
  let str = "\u00c1\u00e1\u00cd\u00ed\u00d3\u00f3\u00da\u00fa\u00e7";
  do_check_calc(str, str.length, PDU_NL_IDENTIFIER_DEFAULT, PDU_NL_IDENTIFIER_DEFAULT, true);

  run_next_test();
});





add_test(function test_RadioInterfaceLayer__calculateUserDataLength() {
  let ril = newRadioInterfaceLayer();

  function test_calc(str, expected, enabledGsmTableTuples) {
    ril.enabledGsmTableTuples = enabledGsmTableTuples;
    let options = ril._calculateUserDataLength(str, expected[5]);

    do_check_eq(str, options.fullBody);
    do_check_eq(expected[0], options.dcs);
    do_check_eq(expected[1], options.encodedFullBodyLength);
    do_check_eq(expected[2], options.userDataHeaderLength);
    do_check_eq(expected[3], options.langIndex);
    do_check_eq(expected[4], options.langShiftIndex);
    do_check_eq(expected[5], options.strict7BitEncoding);
  }

  
  
  test_calc("A", [PDU_DCS_MSG_CODING_16BITS_ALPHABET, 2, 0,], []);
  
  test_calc("A", [PDU_DCS_MSG_CODING_16BITS_ALPHABET, 2, 0,], [[2, 2]]);

  
  test_calc("A", [PDU_DCS_MSG_CODING_7BITS_ALPHABET, 1, 0, 0, 0], [[0, 0]]);
  
  
  test_calc(SP, [PDU_DCS_MSG_CODING_7BITS_ALPHABET, 1, 0, 0, 0], [[0, 0]]);
  
  
  test_calc("^", [PDU_DCS_MSG_CODING_7BITS_ALPHABET, 2, 0, 0, 0], [[0, 0]]);

  
  
  test_calc("A", [PDU_DCS_MSG_CODING_7BITS_ALPHABET, 1, 6, 1, 1], [[1, 1]]);
  
  test_calc("A", [PDU_DCS_MSG_CODING_7BITS_ALPHABET, 1, 3, 1, 0], [[1, 0]]);
  
  test_calc("^", [PDU_DCS_MSG_CODING_7BITS_ALPHABET, 2, 3, 0, 1], [[0, 1]]);

  
  
  test_calc("A", [PDU_DCS_MSG_CODING_7BITS_ALPHABET, 1, 3, 1, 0], [[1, 0], [2, 0]]);
  test_calc("A", [PDU_DCS_MSG_CODING_7BITS_ALPHABET, 1, 3, 1, 0], [[2, 0], [1, 0]]);
  
  test_calc("A", [PDU_DCS_MSG_CODING_7BITS_ALPHABET, 2, 6, 2, 4], [[2, 0], [2, 4]]);
  test_calc("A", [PDU_DCS_MSG_CODING_7BITS_ALPHABET, 2, 6, 2, 4], [[2, 4], [2, 0]]);
  
  
  test_calc("A", [PDU_DCS_MSG_CODING_7BITS_ALPHABET, 1, 3, 1, 0], [[1, 0], [2, 4]]);
  test_calc("A", [PDU_DCS_MSG_CODING_7BITS_ALPHABET, 1, 3, 1, 0], [[2, 4], [1, 0]]);

  
  
  
  
  
  
  
  test_calc("\\\\\\\\\\\\\\",
            [PDU_DCS_MSG_CODING_7BITS_ALPHABET, 14, 0, 0, 0], [[3, 1], [0, 0]]);
  
  
  
  test_calc(ESCAPE + ESCAPE + ESCAPE + ESCAPE + ESCAPE + "\\",
            [PDU_DCS_MSG_CODING_7BITS_ALPHABET, 2, 0, 0, 0], [[3, 0], [0, 0]]);

  
  let str = "\u00c1\u00e1\u00cd\u00ed\u00d3\u00f3\u00da\u00fa\u00e7";
  test_calc(str, [PDU_DCS_MSG_CODING_7BITS_ALPHABET, str.length, 0, 0, 0, true], [[0, 0]]);
  test_calc(str, [PDU_DCS_MSG_CODING_16BITS_ALPHABET, str.length * 2, 0], [[0, 0]]);

  run_next_test();
});

function generateStringOfLength(str, length) {
  while (str.length < length) {
    if (str.length < (length / 2)) {
      str = str + str;
    } else {
      str = str + str.substr(0, length - str.length);
    }
  }

  return str;
}




add_test(function test_RadioInterfaceLayer__calculateUserDataLength7Bit_multipart() {
  let ril = newRadioInterfaceLayer();

  function test_calc(str, expected) {
    let options = ril._calculateUserDataLength7Bit(str);

    do_check_eq(expected[0], options.encodedFullBodyLength);
    do_check_eq(expected[1], options.userDataHeaderLength);
    do_check_eq(expected[2], options.segmentMaxSeq);
  }

  test_calc(generateStringOfLength("A", PDU_MAX_USER_DATA_7BIT),
            [PDU_MAX_USER_DATA_7BIT, 0, 1]);
  test_calc(generateStringOfLength("A", PDU_MAX_USER_DATA_7BIT + 1),
            [PDU_MAX_USER_DATA_7BIT + 1, 5, 2]);

  run_next_test();
});




add_test(function test_RadioInterfaceLayer__fragmentText7Bit() {
  let ril = newRadioInterfaceLayer();

  function test_calc(str, strict7BitEncoding, expected) {
    let options = ril._fragmentText(str, null, strict7BitEncoding);
    if (expected) {
      do_check_eq(expected, options.segments.length);
    } else {
      do_check_eq(null, options.segments);
    }
  }

  

  
  test_calc("", false);
  test_calc("", true);
  test_calc(generateStringOfLength("A", PDU_MAX_USER_DATA_7BIT), false);
  test_calc(generateStringOfLength("A", PDU_MAX_USER_DATA_7BIT), true);
  test_calc(generateStringOfLength("A", PDU_MAX_USER_DATA_7BIT + 1), false, 2);
  test_calc(generateStringOfLength("A", PDU_MAX_USER_DATA_7BIT + 1), true, 2);

  
  test_calc(generateStringOfLength("{", PDU_MAX_USER_DATA_7BIT / 2), false);
  test_calc(generateStringOfLength("{", PDU_MAX_USER_DATA_7BIT / 2 + 1), false, 2);
  
  test_calc(generateStringOfLength("{", (PDU_MAX_USER_DATA_7BIT - 7) * 2 / 2), false, 3);

  
  test_calc(generateStringOfLength("A", PDU_MAX_USER_DATA_7BIT - 7));
  test_calc(generateStringOfLength("A", (PDU_MAX_USER_DATA_7BIT - 7) * 2), false, 2);
  test_calc(generateStringOfLength("A", (PDU_MAX_USER_DATA_7BIT - 7) * 3), false, 3);

  

  
  test_calc(generateStringOfLength("\ua2db", PDU_MAX_USER_DATA_UCS2));
  test_calc(generateStringOfLength("\ua2db", PDU_MAX_USER_DATA_UCS2), true);
  test_calc(generateStringOfLength("\ua2db", PDU_MAX_USER_DATA_UCS2 + 1), false, 2);
  test_calc(generateStringOfLength("\ua2db", PDU_MAX_USER_DATA_UCS2 + 1), true, 2);

  
  ril.segmentRef16Bit = true;
  test_calc(generateStringOfLength("\ua2db", (PDU_MAX_USER_DATA_UCS2 * 2 - 7) * 2 / 2), false, 3);
  ril.segmentRef16Bit = false;

  
  let str = "\u00c1\u00e1\u00cd\u00ed\u00d3\u00f3\u00da\u00fa\u00e7";
  for (let i = 0; i < str.length; i++) {
    let c = str.charAt(i);
    test_calc(generateStringOfLength(c, PDU_MAX_USER_DATA_7BIT), false, 3);
    test_calc(generateStringOfLength(c, PDU_MAX_USER_DATA_7BIT), true);
    test_calc(generateStringOfLength(c, PDU_MAX_USER_DATA_UCS2), false);
  }

  run_next_test();
});




add_test(function test_GsmPDUHelper_writeStringAsSeptets() {
  let worker = newWorker({
    postRILMessage: function fakePostRILMessage(data) {
      
    },
    postMessage: function fakePostMessage(message) {
      
    }
  });

  let helper = worker.GsmPDUHelper;
  helper.resetOctetWritten = function () {
    helper.octetsWritten = 0;
  };
  helper.writeHexOctet = function () {
    helper.octetsWritten++;
  };

  let base = "AAAAAAAA"; 
  for (let len = 0; len < 8; len++) {
    let str = base.substring(0, len);

    for (let paddingBits = 0; paddingBits < 8; paddingBits++) {
      do_print("Verifying GsmPDUHelper.writeStringAsSeptets("
               + str + ", " + paddingBits + ", <default>, <default>)");
      helper.resetOctetWritten();
      helper.writeStringAsSeptets(str, paddingBits, PDU_NL_IDENTIFIER_DEFAULT,
                                  PDU_NL_IDENTIFIER_DEFAULT);
      do_check_eq(Math.ceil(((len * 7) + paddingBits) / 8),
                  helper.octetsWritten);
    }
  }

  
  let str = "\u00c1\u00e1\u00cd\u00ed\u00d3\u00f3\u00da\u00fa\u00e7";
  helper.resetOctetWritten();
  do_print("Verifying GsmPDUHelper.writeStringAsSeptets(" + str + ", 0, <default>, <default>, true)");
  helper.writeStringAsSeptets(str, 0, PDU_NL_IDENTIFIER_DEFAULT, PDU_NL_IDENTIFIER_DEFAULT, true);
  do_check_eq(Math.ceil(str.length * 7 / 8), helper.octetsWritten);

  run_next_test();
});





function hexToNibble(nibble) {
  nibble &= 0x0f;
  if (nibble < 10) {
    nibble += 48; 
  } else {
    nibble += 55; 
  }
  return nibble;
}

function pduToParcelData(pdu) {
  let dataLength = 4 + pdu.length * 4 + 4;
  let data = new Uint8Array(dataLength);
  let offset = 0;

  
  data[offset++] = pdu.length & 0xFF;
  data[offset++] = (pdu.length >> 8) & 0xFF;
  data[offset++] = (pdu.length >> 16) & 0xFF;
  data[offset++] = (pdu.length >> 24) & 0xFF;

  
  for (let i = 0; i < pdu.length; i++) {
    let hi = (pdu[i] >>> 4) & 0x0F;
    let lo = pdu[i] & 0x0F;

    data[offset++] = hexToNibble(hi);
    data[offset++] = 0;
    data[offset++] = hexToNibble(lo);
    data[offset++] = 0;
  }

  
  data[offset++] = 0;
  data[offset++] = 0;
  data[offset++] = 0;
  data[offset++] = 0;

  return data;
}

function compose7bitPdu(lst, sst, data, septets) {
  if ((lst == 0) && (sst == 0)) {
    return [0x00,                              
            PDU_MTI_SMS_DELIVER,               
            1, 0x00, 0,                        
            0x00,                              
            PDU_DCS_MSG_CODING_7BITS_ALPHABET, 
            0, 0, 0, 0, 0, 0, 0,               
            septets]                           
           .concat(data);
  }

  return [0x00,                                            
          PDU_MTI_SMS_DELIVER | PDU_UDHI,                  
          1, 0x00, 0,                                      
          0x00,                                            
          PDU_DCS_MSG_CODING_7BITS_ALPHABET,               
          0, 0, 0, 0, 0, 0, 0,                             
          8 + septets,                                     
          6,                                               
          PDU_IEI_NATIONAL_LANGUAGE_LOCKING_SHIFT, 1, lst, 
          PDU_IEI_NATIONAL_LANGUAGE_SINGLE_SHIFT, 1, sst]  
         .concat(data);
}

function composeUcs2Pdu(rawBytes) {
  return [0x00,                               
          PDU_MTI_SMS_DELIVER,                
          1, 0x00, 0,                         
          0x00,                               
          PDU_DCS_MSG_CODING_16BITS_ALPHABET, 
          0, 0, 0, 0, 0, 0, 0,                
          rawBytes.length]                    
         .concat(rawBytes);
}

function newSmsParcel(pdu) {
  return newIncomingParcel(-1,
                           RESPONSE_TYPE_UNSOLICITED,
                           UNSOLICITED_RESPONSE_NEW_SMS,
                           pduToParcelData(pdu));
}

function removeSpecialChar(str, needle) {
  for (let i = 0; i < needle.length; i++) {
    let pos;
    while ((pos = str.indexOf(needle[i])) >= 0) {
      str = str.substring(0, pos) + str.substring(pos + 1);
    }
  }
  return str;
}

function newWriteHexOctetAsUint8Worker() {
  let worker = newWorker({
    postRILMessage: function fakePostRILMessage(data) {
      
    },
    postMessage: function fakePostMessage(message) {
      
    }
  });

  worker.GsmPDUHelper.writeHexOctet = function (value) {
    worker.Buf.writeUint8(value);
  };

  return worker;
}

function add_test_receiving_sms(expected, pdu) {
  add_test(function test_receiving_sms() {
    let worker = newWorker({
      postRILMessage: function fakePostRILMessage(data) {
        
      },
      postMessage: function fakePostMessage(message) {
        do_print("body: " + message.body);
        do_check_eq(expected, message.body)
      }
    });

    do_print("expect: " + expected);
    do_print("pdu: " + pdu);
    worker.onRILMessage(newSmsParcel(pdu));

    run_next_test();
  });
}

function test_receiving_7bit_alphabets(lst, sst) {
  let ril = newRadioInterfaceLayer();

  let worker = newWriteHexOctetAsUint8Worker();
  let helper = worker.GsmPDUHelper;
  let buf = worker.Buf;

  function get7bitRawBytes(expected) {
    buf.outgoingIndex = 0;
    helper.writeStringAsSeptets(expected, 0, lst, sst);

    let subArray = buf.outgoingBytes.subarray(0, buf.outgoingIndex);
    return Array.slice(subArray);
  }

  let langTable = PDU_NL_LOCKING_SHIFT_TABLES[lst];
  let langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[sst];

  let text = removeSpecialChar(langTable + langShiftTable, ESCAPE + RESCTL);
  for (let i = 0; i < text.length;) {
    let len = Math.min(70, text.length - i);
    let expected = text.substring(i, i + len);
    let septets = ril._countGsm7BitSeptets(expected, langTable, langShiftTable);
    let rawBytes = get7bitRawBytes(expected);
    let pdu = compose7bitPdu(lst, sst, rawBytes, septets);
    add_test_receiving_sms(expected, pdu);

    i += len;
  }
}

function test_receiving_ucs2_alphabets(text) {
  let worker = newWriteHexOctetAsUint8Worker();
  let buf = worker.Buf;

  function getUCS2RawBytes(expected) {
    buf.outgoingIndex = 0;
    worker.GsmPDUHelper.writeUCS2String(expected);

    let subArray = buf.outgoingBytes.subarray(0, buf.outgoingIndex);
    return Array.slice(subArray);
  }

  for (let i = 0; i < text.length;) {
    let len = Math.min(70, text.length - i);
    let expected = text.substring(i, i + len);
    let rawBytes = getUCS2RawBytes(expected);
    let pdu = composeUcs2Pdu(rawBytes);
    add_test_receiving_sms(expected, pdu);

    i += len;
  }
}

let ucs2str = "";
for (let lst = 0; lst < PDU_NL_LOCKING_SHIFT_TABLES.length; lst++) {
  ucs2str += PDU_NL_LOCKING_SHIFT_TABLES[lst];
  for (let sst = 0; sst < PDU_NL_SINGLE_SHIFT_TABLES.length; sst++) {
    test_receiving_7bit_alphabets(lst, sst);

    if (lst == 0) {
      ucs2str += PDU_NL_SINGLE_SHIFT_TABLES[sst];
    }
  }
}
test_receiving_ucs2_alphabets(ucs2str);


add_test(function test_sendSMS_UCS2_without_langIndex_langShiftIndex_defined() {
  let worker = newWriteHexOctetAsUint8Worker();

  worker.Buf.sendParcel = function () {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    do_check_eq(this.outgoingIndex, 57);

    run_next_test();
  };

  worker.RIL.sendSMS({
    number: "1",
    segmentMaxSeq: 2,
    fullBody: "Hello World!",
    dcs: PDU_DCS_MSG_CODING_16BITS_ALPHABET,
    segmentRef16Bit: false,
    userDataHeaderLength: 5,
    strict7BitEncoding: false,
    requestStatusReport: true,
    segments: [
      {
        body: "Hello ",
        encodedBodyLength: 12,
      }, {
        body: "World!",
        encodedBodyLength: 12,
      }
    ],
  });
});




add_test(function test_GsmPDUHelper_readAddress() {
  let worker = newWorker({
    postRILMessage: function fakePostRILMessage(data) {
      
    },
    postMessage: function fakePostMessage(message) {
      
    }

  });

  let helper = worker.GsmPDUHelper;
  function test_address(addrHex, addrString) {
    let uint16Array = [];
    let ix = 0;
    for (let i = 0; i < addrHex.length; ++i) {
      uint16Array[i] = addrHex[i].charCodeAt();
    }

    worker.Buf.readUint16 = function (){
      if(ix >= uint16Array.length) {
        do_throw("out of range in uint16Array");
      }
      return uint16Array[ix++];
    }
    let length = helper.readHexOctet();
    let parsedAddr = helper.readAddress(length);
    do_check_eq(parsedAddr, addrString);
  }

  
  test_address("04D01100", "_@");
  test_address("04D01000", "\u0394@");

  
  test_address("0B914151245584F6", "+14154255486");
  test_address("0E914151245584B633", "+14154255486#33");

  
  test_address("0BA14151245584F6", "14154255486");

  run_next_test();
});
