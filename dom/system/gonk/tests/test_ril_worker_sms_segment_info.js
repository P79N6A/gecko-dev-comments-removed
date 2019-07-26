


subscriptLoader.loadSubScript("resource://gre/modules/ril_consts.js", this);

const ESCAPE = "\uffff";
const RESCTL = "\ufffe";
const SP = " ";

function run_test() {
  run_next_test();
}





add_test(function test_RadioInterface__countGsm7BitSeptets() {
  let ril = newRadioInterface();

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

  function do_check_calc(str, expectedCalcLen, lst, sst, strict7BitEncoding, strToWrite) {
    do_check_eq(expectedCalcLen,
                ril._countGsm7BitSeptets(str,
                                         PDU_NL_LOCKING_SHIFT_TABLES[lst],
                                         PDU_NL_SINGLE_SHIFT_TABLES[sst],
                                         strict7BitEncoding));

    helper.resetOctetWritten();
    strToWrite = strToWrite || str;
    helper.writeStringAsSeptets(strToWrite, 0, lst, sst);
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

  
  let str = "", strToWrite = "", gsmLen = 0;
  for (let c in GSM_SMS_STRICT_7BIT_CHARMAP) {
    str += c;
    strToWrite += GSM_SMS_STRICT_7BIT_CHARMAP[c];
    if (PDU_NL_LOCKING_SHIFT_TABLES.indexOf(GSM_SMS_STRICT_7BIT_CHARMAP[c])) {
      gsmLen += 1;
    } else {
      gsmLen += 2;
    }
  }
  do_check_calc(str, gsmLen,
                PDU_NL_IDENTIFIER_DEFAULT, PDU_NL_IDENTIFIER_DEFAULT,
                true, strToWrite);

  run_next_test();
});





add_test(function test_RadioInterface__calculateUserDataLength() {
  let ril = newRadioInterface();

  function test_calc(str, expected, enabledGsmTableTuples, strict7BitEncoding) {
    ril.enabledGsmTableTuples = enabledGsmTableTuples;
    let options = ril._calculateUserDataLength(str, strict7BitEncoding);

    do_check_eq(expected[0], options.dcs);
    do_check_eq(expected[1], options.encodedFullBodyLength);
    do_check_eq(expected[2], options.userDataHeaderLength);
    do_check_eq(expected[3], options.langIndex);
    do_check_eq(expected[4], options.langShiftIndex);
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

  
  let str = "", gsmLen = 0, udhl = 0;
  for (let c in GSM_SMS_STRICT_7BIT_CHARMAP) {
    str += c;
    if (PDU_NL_LOCKING_SHIFT_TABLES.indexOf(GSM_SMS_STRICT_7BIT_CHARMAP[c])) {
      gsmLen += 1;
    } else {
      gsmLen += 2;
    }
  }
  if (str.length > PDU_MAX_USER_DATA_UCS2) {
    udhl = 5;
  }
  test_calc(str, [PDU_DCS_MSG_CODING_7BITS_ALPHABET, gsmLen, 0, 0, 0], [[0, 0]], true);
  test_calc(str, [PDU_DCS_MSG_CODING_16BITS_ALPHABET, str.length * 2, udhl], [[0, 0]]);

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




add_test(function test_RadioInterface__calculateUserDataLength7Bit_multipart() {
  let ril = newRadioInterface();

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




add_test(function test_RadioInterface__fragmentText7Bit() {
  let ril = newRadioInterface();

  function test_calc(str, strict7BitEncoding, expectedSegments) {
    expectedSegments = expectedSegments || 1;
    let options = ril._fragmentText(str, null, strict7BitEncoding);
    do_check_eq(expectedSegments, options.segments.length);
  }

  

  
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
  
  
  test_calc(generateStringOfLength("\ua2db", PDU_MAX_USER_DATA_UCS2 + 1), true, 1);

  
  ril.segmentRef16Bit = true;
  test_calc(generateStringOfLength("\ua2db", (PDU_MAX_USER_DATA_UCS2 * 2 - 7) * 2 / 2), false, 3);
  ril.segmentRef16Bit = false;

  
  for (let c in GSM_SMS_STRICT_7BIT_CHARMAP) {
    test_calc(generateStringOfLength(c, PDU_MAX_USER_DATA_7BIT), false, 3);
    test_calc(generateStringOfLength(c, PDU_MAX_USER_DATA_7BIT), true);
    test_calc(generateStringOfLength(c, PDU_MAX_USER_DATA_UCS2), false);
  }

  run_next_test();
});
