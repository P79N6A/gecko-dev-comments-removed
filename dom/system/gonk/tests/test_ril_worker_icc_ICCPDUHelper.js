


subscriptLoader.loadSubScript("resource://gre/modules/ril_consts.js", this);

function run_test() {
  run_next_test();
}




add_test(function test_read_icc_ucs2_string() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let helper = context.GsmPDUHelper;
  let iccHelper = context.ICCPDUHelper;

  
  let text = "TEST";
  helper.writeUCS2String(text);
  
  let ffLen = 2;
  for (let i = 0; i < ffLen; i++) {
    helper.writeHexOctet(0xff);
  }
  equal(iccHelper.readICCUCS2String(0x80, (2 * text.length) + ffLen), text);

  
  let array = [0x08, 0xd2, 0x4d, 0x6f, 0x7a, 0x69, 0x6c, 0x6c, 0x61, 0xca,
               0xff, 0xff];
  let len = array.length;
  for (let i = 0; i < len; i++) {
    helper.writeHexOctet(array[i]);
  }
  equal(iccHelper.readICCUCS2String(0x81, len), "Mozilla\u694a");

  
  let array2 = [0x08, 0x69, 0x00, 0x4d, 0x6f, 0x7a, 0x69, 0x6c, 0x6c, 0x61,
                0xca, 0xff, 0xff];
  let len2 = array2.length;
  for (let i = 0; i < len2; i++) {
    helper.writeHexOctet(array2[i]);
  }
  equal(iccHelper.readICCUCS2String(0x82, len2), "Mozilla\u694a");

  run_next_test();
});




add_test(function test_write_icc_ucs2_string() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let helper = context.GsmPDUHelper;
  let iccHelper = context.ICCPDUHelper;
  let alphaLen = 18;
  let test_data = [
    {
      encode: 0x80,
      
      data: "\u82b3"
    }, {
      encode: 0x80,
      
      data: "Fire \u82b3\u8233"
    }, {
      encode: 0x80,
      
      data: "\u694a\u704a"
    }, {
      encode: 0x81,
      
      data: "Fire \u6901\u697f"
    }, {
      encode: 0x81,
      
      data: "Fire \u6980\u69ff"
    }, {
      encode: 0x82,
      
      data: "Fire \u0514\u0593"
    }, {
      encode: 0x82,
      
      data: "Fire \u8000\u8001"
    }, {
      encode: 0x82,
      
      data: "Fire \ufffd\ufffe"
    }];

  for (let i = 0; i < test_data.length; i++) {
    let test = test_data[i];
    iccHelper.writeICCUCS2String(alphaLen, test.data);
    equal(helper.readHexOctet(), test.encode);
    equal(iccHelper.readICCUCS2String(test.encode, alphaLen - 1), test.data);
  }

  run_next_test();
});



add_test(function test_read_dialling_number() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let helper = context.GsmPDUHelper;
  let iccHelper = context.ICCPDUHelper;
  let str = "123456789";

  helper.readHexOctet = function() {
    return 0x81;
  };

  helper.readSwappedNibbleExtendedBcdString = function(len) {
    return str.substring(0, len);
  };

  for (let i = 0; i < str.length; i++) {
    equal(str.substring(0, i - 1), 
                iccHelper.readDiallingNumber(i));
  }

  run_next_test();
});




add_test(function test_read_8bit_unpacked_to_string() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let helper = context.GsmPDUHelper;
  let iccHelper = context.ICCPDUHelper;
  const langTable = PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
  const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];

  
  
  for (let i = 0; i < PDU_NL_EXTENDED_ESCAPE; i++) {
    helper.writeHexOctet(i);
  }

  
  helper.writeHexOctet(PDU_NL_EXTENDED_ESCAPE);
  helper.writeHexOctet(PDU_NL_EXTENDED_ESCAPE);

  for (let i = PDU_NL_EXTENDED_ESCAPE + 1; i < langTable.length; i++) {
    helper.writeHexOctet(i);
  }

  
  let ffLen = 2;
  for (let i = 0; i < ffLen; i++) {
    helper.writeHexOctet(0xff);
  }

  equal(iccHelper.read8BitUnpackedToString(PDU_NL_EXTENDED_ESCAPE),
              langTable.substring(0, PDU_NL_EXTENDED_ESCAPE));
  equal(iccHelper.read8BitUnpackedToString(2), " ");
  equal(iccHelper.read8BitUnpackedToString(langTable.length -
                                              PDU_NL_EXTENDED_ESCAPE - 1 + ffLen),
              langTable.substring(PDU_NL_EXTENDED_ESCAPE + 1));

  
  for (let i = 0; i < langShiftTable.length; i++) {
    helper.writeHexOctet(PDU_NL_EXTENDED_ESCAPE);
    helper.writeHexOctet(i);
  }

  
  equal(iccHelper.read8BitUnpackedToString(PDU_NL_RESERVED_CONTROL  * 2),
              langShiftTable.substring(0, PDU_NL_RESERVED_CONTROL));
  
  equal(iccHelper.read8BitUnpackedToString(2), " ");
  
  equal(iccHelper.read8BitUnpackedToString(
                (PDU_NL_EXTENDED_ESCAPE - PDU_NL_RESERVED_CONTROL - 1)  * 2),
              langShiftTable.substring(PDU_NL_RESERVED_CONTROL + 1,
                                       PDU_NL_EXTENDED_ESCAPE));
  
  equal(iccHelper.read8BitUnpackedToString(2), " ");
  
  equal(iccHelper.read8BitUnpackedToString(
                (langShiftTable.length - PDU_NL_EXTENDED_ESCAPE - 1)  * 2),
              langShiftTable.substring(PDU_NL_EXTENDED_ESCAPE + 1));

  run_next_test();
});






add_test(function test_write_string_to_8bit_unpacked() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let helper = context.GsmPDUHelper;
  let iccHelper = context.ICCPDUHelper;
  const langTable = PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
  const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
  
  let ffLen = 2;
  let str;

  
  iccHelper.writeStringTo8BitUnpacked(langTable.length + ffLen, langTable);

  for (let i = 0; i < langTable.length; i++) {
    equal(helper.readHexOctet(), i);
  }

  for (let i = 0; i < ffLen; i++) {
    equal(helper.readHexOctet(), 0xff);
  }

  
  str = "\u000c\u20ac";
  iccHelper.writeStringTo8BitUnpacked(4, str);

  equal(iccHelper.read8BitUnpackedToString(4), str);

  
  
  
  str = "\u000c\u20ac\u00a3";

  
  
  
  
  iccHelper.writeStringTo8BitUnpacked(7, str);

  equal(iccHelper.read8BitUnpackedToString(7), str);

  run_next_test();
});




add_test(function test_write_string_to_8bit_unpacked_with_max_octets_written() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let helper = context.GsmPDUHelper;
  let iccHelper = context.ICCPDUHelper;
  const langTable = PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
  const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];

  
  
  iccHelper.writeStringTo8BitUnpacked(3, langTable.substring(0, 4));
  helper.writeHexOctet(0xff); 
  for (let i = 0; i < 3; i++) {
    equal(helper.readHexOctet(), i);
  }
  ok(helper.readHexOctet() != 4);

  
  
  let str = "\u000c\u00a3";
  iccHelper.writeStringTo8BitUnpacked(3, str);
  equal(iccHelper.read8BitUnpackedToString(3), str);

  str = "\u00a3\u000c";
  iccHelper.writeStringTo8BitUnpacked(3, str);
  equal(iccHelper.read8BitUnpackedToString(3), str);

  
  
  str = "\u000c\u000c";
  iccHelper.writeStringTo8BitUnpacked(3, str);
  helper.writeHexOctet(0xff); 
  equal(iccHelper.read8BitUnpackedToString(4), str.substring(0, 1));

  run_next_test();
});




add_test(function test_read_alpha_identifier() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let helper = context.GsmPDUHelper;
  let iccHelper = context.ICCPDUHelper;

  
  let text = "TEST";
  helper.writeHexOctet(0x80);
  helper.writeUCS2String(text);
  
  let ffLen = 2;
  for (let i = 0; i < ffLen; i++) {
    helper.writeHexOctet(0xff);
  }
  equal(iccHelper.readAlphaIdentifier(1 + (2 * text.length) + ffLen), text);

  
  let array = [0x81, 0x08, 0xd2, 0x4d, 0x6f, 0x7a, 0x69, 0x6c, 0x6c, 0x61, 0xca, 0xff, 0xff];
  for (let i = 0; i < array.length; i++) {
    helper.writeHexOctet(array[i]);
  }
  equal(iccHelper.readAlphaIdentifier(array.length), "Mozilla\u694a");

  
  let array2 = [0x82, 0x08, 0x69, 0x00, 0x4d, 0x6f, 0x7a, 0x69, 0x6c, 0x6c, 0x61, 0xca, 0xff, 0xff];
  for (let i = 0; i < array2.length; i++) {
    helper.writeHexOctet(array2[i]);
  }
  equal(iccHelper.readAlphaIdentifier(array2.length), "Mozilla\u694a");

  
  const langTable = PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
  for (let i = 0; i < PDU_NL_EXTENDED_ESCAPE; i++) {
    helper.writeHexOctet(i);
  }
  equal(iccHelper.readAlphaIdentifier(PDU_NL_EXTENDED_ESCAPE),
              langTable.substring(0, PDU_NL_EXTENDED_ESCAPE));

  run_next_test();
});




add_test(function test_write_alpha_identifier() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let helper = context.GsmPDUHelper;
  let iccHelper = context.ICCPDUHelper;
  
  let ffLen = 2;

  
  iccHelper.writeAlphaIdentifier(10, null);
  equal(iccHelper.readAlphaIdentifier(10), "");

  
  let str = "Mozilla";
  iccHelper.writeAlphaIdentifier(str.length + ffLen, str);
  equal(iccHelper.readAlphaIdentifier(str.length + ffLen), str);

  
  str = "Mozilla\u8000";
  iccHelper.writeAlphaIdentifier(str.length * 2 + ffLen, str);
  
  equal(iccHelper.readAlphaIdentifier(str.length * 2 + ffLen), str);

  
  
  str = "\u694a";
  iccHelper.writeAlphaIdentifier(3, str);
  equal(iccHelper.readAlphaIdentifier(3), str);

  
  
  str = "\u694a\u69ca";
  iccHelper.writeAlphaIdentifier(4, str);
  helper.writeHexOctet(0xff); 
  equal(iccHelper.readAlphaIdentifier(5), str.substring(0, 1));

  
  iccHelper.writeAlphaIdentifier(0, "1");
  helper.writeHexOctet(0xff); 
  equal(iccHelper.readAlphaIdentifier(1), "");

  run_next_test();
});




add_test(function test_read_alpha_id_dialling_number() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let helper = context.GsmPDUHelper;
  let iccHelper = context.ICCPDUHelper;
  let buf = context.Buf;
  const recordSize = 32;

  function testReadAlphaIdDiallingNumber(contact) {
    iccHelper.readAlphaIdentifier = function() {
      return contact.alphaId;
    };

    iccHelper.readNumberWithLength = function() {
      return contact.number;
    };

    let strLen = recordSize * 2;
    buf.writeInt32(strLen);     
    helper.writeHexOctet(0xff); 
    helper.writeHexOctet(0xff); 
    buf.writeStringDelimiter(strLen);

    let contactR = iccHelper.readAlphaIdDiallingNumber(recordSize);
    if (contact.alphaId == "" && contact.number == "") {
      equal(contactR, null);
    } else {
      equal(contactR.alphaId, contact.alphaId);
      equal(contactR.number, contact.number);
    }
  }

  testReadAlphaIdDiallingNumber({alphaId: "AlphaId", number: "0987654321"});
  testReadAlphaIdDiallingNumber({alphaId: "", number: ""});

  run_next_test();
});




add_test(function test_write_alpha_id_dialling_number() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let helper = context.ICCPDUHelper;
  const recordSize = 32;

  
  let contactW = {
    alphaId: "Mozilla",
    number: "1234567890"
  };
  helper.writeAlphaIdDiallingNumber(recordSize, contactW.alphaId,
                                    contactW.number);

  let contactR = helper.readAlphaIdDiallingNumber(recordSize);
  equal(contactW.alphaId, contactR.alphaId);
  equal(contactW.number, contactR.number);

  
  let contactUCS2 = {
    alphaId: "火狐",
    number: "+1234567890"
  };
  helper.writeAlphaIdDiallingNumber(recordSize, contactUCS2.alphaId,
                                    contactUCS2.number);
  contactR = helper.readAlphaIdDiallingNumber(recordSize);
  equal(contactUCS2.alphaId, contactR.alphaId);
  equal(contactUCS2.number, contactR.number);

  
  helper.writeAlphaIdDiallingNumber(recordSize);
  contactR = helper.readAlphaIdDiallingNumber(recordSize);
  equal(contactR, null);

  
  
  
  
  
  let longContact = {
    alphaId: "AAAAAAAAABBBBBBBBBCCCCCCCCC",
    number: "123456789012345678901234567890",
  };
  helper.writeAlphaIdDiallingNumber(recordSize, longContact.alphaId,
                                    longContact.number);
  contactR = helper.readAlphaIdDiallingNumber(recordSize);
  equal(contactR.alphaId, "AAAAAAAAABBBBBBBBB");
  equal(contactR.number, "12345678901234567890");

  
  longContact.number = "+123456789012345678901234567890";
  helper.writeAlphaIdDiallingNumber(recordSize, longContact.alphaId,
                                    longContact.number);
  contactR = helper.readAlphaIdDiallingNumber(recordSize);
  equal(contactR.alphaId, "AAAAAAAAABBBBBBBBB");
  equal(contactR.number, "+12345678901234567890");

  run_next_test();
});




add_test(function test_write_dialling_number() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let helper = context.ICCPDUHelper;

  
  let number = "+123456";
  let len = 4;
  helper.writeDiallingNumber(number);
  equal(helper.readDiallingNumber(len), number);

  
  number = "987654";
  len = 4;
  helper.writeDiallingNumber(number);
  equal(helper.readDiallingNumber(len), number);

  number = "9876543";
  len = 5;
  helper.writeDiallingNumber(number);
  equal(helper.readDiallingNumber(len), number);

  run_next_test();
});




add_test(function test_read_number_with_length() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let helper = context.GsmPDUHelper;
  let iccHelper = context.ICCPDUHelper;

  let numbers = [
    {
      number: "123456789",
      expectedNumber: "123456789"
    },
    {
      number: null,
      expectedNumber: null
    },
    
    {
      number: "12345678901234567890123",
      expectedNumber: ""
    },
  ];

  
  context.Buf.seekIncoming = function(offset) {
  };

  function do_test(aNumber, aExpectedNumber) {
    iccHelper.readDiallingNumber = function(numLen) {
      return aNumber.substring(0, numLen);
    };

    if (aNumber != null) {
      helper.writeHexOctet(aNumber.length + 1);
    } else {
      helper.writeHexOctet(0xff);
    }

    equal(iccHelper.readNumberWithLength(), aExpectedNumber);
  }

  for (let i = 0; i < numbers.length; i++) {
    do_test(numbers[i].number, numbers[i].expectedNumber);
  }

  run_next_test();
});




add_test(function test_write_number_with_length() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let helper = context.GsmPDUHelper;
  let iccHelper = context.ICCPDUHelper;

  function test(number, expectedNumber) {
    expectedNumber = expectedNumber || number;
    iccHelper.writeNumberWithLength(number);
    let numLen = helper.readHexOctet();
    equal(expectedNumber, iccHelper.readDiallingNumber(numLen));
    for (let i = 0; i < (ADN_MAX_BCD_NUMBER_BYTES - numLen); i++) {
      equal(0xff, helper.readHexOctet());
    }
  }

  
  test("123456789");

  
  test("+987654321");

  
  test("1*2#3,4*5#6,");

  
  test("+1*2#3,4*5#6,");

  
  test("(1)23-456+789", "123456789");

  test("++(01)2*3-4#5,6+7(8)9*0#1,", "+012*34#5,6789*0#1,");

  
  iccHelper.writeNumberWithLength(null);
  for (let i = 0; i < (ADN_MAX_BCD_NUMBER_BYTES + 1); i++) {
    equal(0xff, helper.readHexOctet());
  }

  run_next_test();
});
