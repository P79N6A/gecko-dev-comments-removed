


subscriptLoader.loadSubScript("resource://gre/modules/ril_consts.js", this);

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "gSmsSegmentHelper", function() {
  let ns = {};
  Cu.import("resource://gre/modules/SmsSegmentHelper.jsm", ns);
  return ns.SmsSegmentHelper;
});

const ESCAPE = "\uffff";
const RESCTL = "\ufffe";
const SP = " ";

function run_test() {
  run_next_test();
}





add_test(function test_SmsSegmentHelper_calculateUserDataLength() {
  function test_calc(str, expected, enabledGsmTableTuples, strict7BitEncoding) {
    gSmsSegmentHelper.enabledGsmTableTuples = enabledGsmTableTuples;
    let options = gSmsSegmentHelper.calculateUserDataLength(str, strict7BitEncoding);

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




add_test(function test_SmsSegmentHelper_calculateUserDataLength7Bit_multipart() {
  function test_calc(str, expected) {
    let options = gSmsSegmentHelper.calculateUserDataLength7Bit(str);

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




add_test(function test_SmsSegmentHelper_fragmentText7Bit() {
  function test_calc(str, strict7BitEncoding, expectedSegments) {
    expectedSegments = expectedSegments || 1;
    let options = gSmsSegmentHelper.fragmentText(str, null, strict7BitEncoding);
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

  
  gSmsSegmentHelper.segmentRef16Bit = true;
  test_calc(generateStringOfLength("\ua2db", (PDU_MAX_USER_DATA_UCS2 * 2 - 7) * 2 / 2), false, 3);
  gSmsSegmentHelper.segmentRef16Bit = false;

  
  for (let c in GSM_SMS_STRICT_7BIT_CHARMAP) {
    test_calc(generateStringOfLength(c, PDU_MAX_USER_DATA_7BIT), false, 3);
    test_calc(generateStringOfLength(c, PDU_MAX_USER_DATA_7BIT), true);
    test_calc(generateStringOfLength(c, PDU_MAX_USER_DATA_UCS2), false);
  }

  run_next_test();
});
