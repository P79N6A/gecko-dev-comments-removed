


subscriptLoader.loadSubScript("resource://gre/modules/ril_consts.js", this);

function run_test() {
  run_next_test();
}




function newUint8Worker() {
  let worker = newWorker();
  let index = 0; 
  let buf = [];

  worker.Buf.writeUint8 = function (value) {
    buf.push(value);
  };

  worker.Buf.readUint8 = function () {
    return buf[index++];
  };

  worker.Buf.seekIncoming = function (offset) {
    index += offset;
  };

  worker.debug = do_print;

  return worker;
}




add_test(function test_read_icc_ucs2_string() {
  let worker = newUint8Worker();
  let helper = worker.GsmPDUHelper;

  
  let text = "TEST";
  helper.writeUCS2String(text);
  
  let ffLen = 2;
  for (let i = 0; i < ffLen; i++) {
    helper.writeHexOctet(0xff);
  }
  do_check_eq(helper.readICCUCS2String(0x80, (2 * text.length) + ffLen), text);

  
  let array = [0x08, 0xd2, 0x4d, 0x6f, 0x7a, 0x69, 0x6c, 0x6c, 0x61, 0xca,
               0xff, 0xff];
  let len = array.length;
  for (let i = 0; i < len; i++) {
    helper.writeHexOctet(array[i]);
  }
  do_check_eq(helper.readICCUCS2String(0x81, len), "Mozilla\u694a");

  
  let array2 = [0x08, 0x69, 0x00, 0x4d, 0x6f, 0x7a, 0x69, 0x6c, 0x6c, 0x61,
                0xca, 0xff, 0xff];
  let len2 = array2.length;
  for (let i = 0; i < len2; i++) {
    helper.writeHexOctet(array2[i]);
  }
  do_check_eq(helper.readICCUCS2String(0x82, len2), "Mozilla\u694a");

  run_next_test();
});




add_test(function test_read_dialling_number() {
  let worker = newUint8Worker();
  let helper = worker.GsmPDUHelper;
  let str = "123456789";

  helper.readHexOctet = function () {
    return 0x81;
  };

  helper.readSwappedNibbleBcdString = function (len) {
    return str.substring(0, len);
  };

  for (let i = 0; i < str.length; i++) {
    do_check_eq(str.substring(0, i - 1), 
                helper.readDiallingNumber(i));
  }

  run_next_test();
});




add_test(function test_read_8bit_unpacked_to_string() {
  let worker = newUint8Worker();
  let helper = worker.GsmPDUHelper;
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

  do_check_eq(helper.read8BitUnpackedToString(PDU_NL_EXTENDED_ESCAPE),
              langTable.substring(0, PDU_NL_EXTENDED_ESCAPE));
  do_check_eq(helper.read8BitUnpackedToString(2), " ");
  do_check_eq(helper.read8BitUnpackedToString(langTable.length -
                                              PDU_NL_EXTENDED_ESCAPE - 1 + ffLen),
              langTable.substring(PDU_NL_EXTENDED_ESCAPE + 1));

  
  for (let i = 0; i < langShiftTable.length; i++) {
    helper.writeHexOctet(PDU_NL_EXTENDED_ESCAPE);
    helper.writeHexOctet(i);
  }

  
  do_check_eq(helper.read8BitUnpackedToString(PDU_NL_RESERVED_CONTROL  * 2),
              langShiftTable.substring(0, PDU_NL_RESERVED_CONTROL));
  
  do_check_eq(helper.read8BitUnpackedToString(2), " ");
  
  do_check_eq(helper.read8BitUnpackedToString(
                (PDU_NL_EXTENDED_ESCAPE - PDU_NL_RESERVED_CONTROL - 1)  * 2),
              langShiftTable.substring(PDU_NL_RESERVED_CONTROL + 1,
                                       PDU_NL_EXTENDED_ESCAPE));
  
  do_check_eq(helper.read8BitUnpackedToString(2), " ");
  
  do_check_eq(helper.read8BitUnpackedToString(
                (langShiftTable.length - PDU_NL_EXTENDED_ESCAPE - 1)  * 2),
              langShiftTable.substring(PDU_NL_EXTENDED_ESCAPE + 1));

  run_next_test();
});






add_test(function test_write_string_to_8bit_unpacked() {
  let worker = newUint8Worker();
  let helper = worker.GsmPDUHelper;
  const langTable = PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
  const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
  
  let ffLen = 2;
  let str;

  
  helper.writeStringTo8BitUnpacked(langTable.length + ffLen, langTable);

  for (let i = 0; i < langTable.length; i++) {
    do_check_eq(helper.readHexOctet(), i);
  }

  for (let i = 0; i < ffLen; i++) {
    do_check_eq(helper.readHexOctet(), 0xff);
  }

  
  str = "\u000c\u20ac";
  helper.writeStringTo8BitUnpacked(4, str);

  do_check_eq(helper.read8BitUnpackedToString(4), str);

  
  
  
  str = "\u000c\u20ac\u00a3";

  
  
  
  
  helper.writeStringTo8BitUnpacked(7, str);

  do_check_eq(helper.read8BitUnpackedToString(7), str);

  run_next_test();
});




add_test(function test_write_string_to_8bit_unpacked_with_max_octets_written() {
  let worker = newUint8Worker();
  let helper = worker.GsmPDUHelper;
  const langTable = PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
  const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];

  
  
  helper.writeStringTo8BitUnpacked(3, langTable.substring(0, 4));
  helper.writeHexOctet(0xff); 
  for (let i = 0; i < 3; i++) {
    do_check_eq(helper.readHexOctet(), i);
  }
  do_check_false(helper.readHexOctet() == 4);

  
  
  let str = "\u000c\u00a3";
  helper.writeStringTo8BitUnpacked(3, str);
  do_check_eq(helper.read8BitUnpackedToString(3), str);

  str = "\u00a3\u000c";
  helper.writeStringTo8BitUnpacked(3, str);
  do_check_eq(helper.read8BitUnpackedToString(3), str);

  
  
  str = "\u000c\u000c";
  helper.writeStringTo8BitUnpacked(3, str);
  helper.writeHexOctet(0xff); 
  do_check_eq(helper.read8BitUnpackedToString(4), str.substring(0, 1));

  run_next_test();
});




add_test(function test_write_alpha_identifier() {
  let worker = newUint8Worker();
  let helper = worker.GsmPDUHelper;
  
  let ffLen = 2;

  
  helper.writeAlphaIdentifier(10, null);
  do_check_eq(helper.readAlphaIdentifier(10), "");

  
  let str = "Mozilla";
  helper.writeAlphaIdentifier(str.length + ffLen, str);
  do_check_eq(helper.readAlphaIdentifier(str.length + ffLen), str);

  
  str = "Mozilla\u694a";
  helper.writeAlphaIdentifier(str.length * 2 + ffLen, str);
  
  do_check_eq(helper.readAlphaIdentifier(str.length * 2 + ffLen), str);

  
  
  str = "\u694a";
  helper.writeAlphaIdentifier(3, str);
  do_check_eq(helper.readAlphaIdentifier(3), str);

  
  
  str = "\u694a\u694a";
  helper.writeAlphaIdentifier(4, str);
  helper.writeHexOctet(0xff); 
  do_check_eq(helper.readAlphaIdentifier(5), str.substring(0, 1));

  
  helper.writeAlphaIdentifier(0, "1");
  helper.writeHexOctet(0xff); 
  do_check_eq(helper.readAlphaIdentifier(1), "");

  run_next_test();
});




add_test(function test_write_alpha_id_dialling_number() {
  let worker = newUint8Worker();
  let helper = worker.GsmPDUHelper;
  const recordSize = 32;

  
  let contactW = {
    alphaId: "Mozilla",
    number: "1234567890"
  };
  helper.writeAlphaIdDiallingNumber(recordSize, contactW.alphaId,
                                    contactW.number);

  let contactR = helper.readAlphaIdDiallingNumber(recordSize);
  do_check_eq(contactW.alphaId, contactR.alphaId);
  do_check_eq(contactW.number, contactR.number);

  
  let contactUCS2 = {
    alphaId: "火狐",
    number: "+1234567890"
  };
  helper.writeAlphaIdDiallingNumber(recordSize, contactUCS2.alphaId,
                                    contactUCS2.number);
  contactR = helper.readAlphaIdDiallingNumber(recordSize);
  do_check_eq(contactUCS2.alphaId, contactR.alphaId);
  do_check_eq(contactUCS2.number, contactR.number);

  
  helper.writeAlphaIdDiallingNumber(recordSize);
  contactR = helper.readAlphaIdDiallingNumber(recordSize);
  do_check_eq(contactR, null);

  
  
  
  
  
  let longContact = {
    alphaId: "AAAAAAAAABBBBBBBBBCCCCCCCCC",
    number: "123456789012345678901234567890",
  };
  helper.writeAlphaIdDiallingNumber(recordSize, longContact.alphaId,
                                    longContact.number);
  contactR = helper.readAlphaIdDiallingNumber(recordSize);
  do_check_eq(contactR.alphaId, "AAAAAAAAABBBBBBBBB");
  do_check_eq(contactR.number, "12345678901234567890");

  
  longContact.number = "+123456789012345678901234567890";
  helper.writeAlphaIdDiallingNumber(recordSize, longContact.alphaId,
                                    longContact.number);
  contactR = helper.readAlphaIdDiallingNumber(recordSize);
  do_check_eq(contactR.alphaId, "AAAAAAAAABBBBBBBBB");
  do_check_eq(contactR.number, "+12345678901234567890");

  run_next_test();
});




add_test(function test_write_dialling_number() {
  let worker = newUint8Worker();
  let helper = worker.GsmPDUHelper;

  
  let number = "+123456";
  let len = 4;
  helper.writeDiallingNumber(number);
  do_check_eq(helper.readDiallingNumber(len), number);

  
  number = "987654";
  len = 4;
  helper.writeDiallingNumber(number);
  do_check_eq(helper.readDiallingNumber(len), number);

  number = "9876543";
  len = 5;
  helper.writeDiallingNumber(number);
  do_check_eq(helper.readDiallingNumber(len), number);

  run_next_test();
});




add_test(function test_write_timestamp() {
  let worker = newUint8Worker();
  let helper = worker.GsmPDUHelper;

  
  let dateInput = new Date();
  let dateOutput = new Date();
  helper.writeTimestamp(dateInput);
  dateOutput.setTime(helper.readTimestamp());

  do_check_eq(dateInput.getFullYear(), dateOutput.getFullYear());
  do_check_eq(dateInput.getMonth(), dateOutput.getMonth());
  do_check_eq(dateInput.getDate(), dateOutput.getDate());
  do_check_eq(dateInput.getHours(), dateOutput.getHours());
  do_check_eq(dateInput.getMinutes(), dateOutput.getMinutes());
  do_check_eq(dateInput.getSeconds(), dateOutput.getSeconds());
  do_check_eq(dateInput.getTimezoneOffset(), dateOutput.getTimezoneOffset());

  
  let time = Date.UTC(2034, 1, 23, 12, 34, 56);
  time = time - (8 * 60 * 60 * 1000);
  dateInput.setTime(time);
  helper.writeTimestamp(dateInput);
  dateOutput.setTime(helper.readTimestamp());

  do_check_eq(dateInput.getFullYear(), dateOutput.getFullYear());
  do_check_eq(dateInput.getMonth(), dateOutput.getMonth());
  do_check_eq(dateInput.getDate(), dateOutput.getDate());
  do_check_eq(dateInput.getHours(), dateOutput.getHours());
  do_check_eq(dateInput.getMinutes(), dateOutput.getMinutes());
  do_check_eq(dateInput.getSeconds(), dateOutput.getSeconds());
  do_check_eq(dateInput.getTimezoneOffset(), dateOutput.getTimezoneOffset());

  run_next_test();
});




add_test(function test_octect_BCD() {
  let worker = newUint8Worker();
  let helper = worker.GsmPDUHelper;

  
  let number = 23;
  let octet = helper.BCDToOctet(number);
  do_check_eq(helper.octetToBCD(octet), number);

  
  number = 56;
  octet = helper.BCDToOctet(number);
  do_check_eq(helper.octetToBCD(octet), number);

  
  octet = 0x23;
  number = helper.octetToBCD(octet);
  do_check_eq(helper.BCDToOctet(number), octet);

  
  octet = 0x56;
  number = helper.octetToBCD(octet);
  do_check_eq(helper.BCDToOctet(number), octet);

  run_next_test();
});




add_test(function test_is_icc_service_available() {
  let worker = newUint8Worker();
  let ICCUtilsHelper = worker.ICCUtilsHelper;

  function test_table(sst, geckoService, simEnabled, usimEnabled) {
    worker.RIL.iccInfoPrivate.sst = sst;
    worker.RIL.appType = CARD_APPTYPE_SIM;
    do_check_eq(ICCUtilsHelper.isICCServiceAvailable(geckoService), simEnabled);
    worker.RIL.appType = CARD_APPTYPE_USIM;
    do_check_eq(ICCUtilsHelper.isICCServiceAvailable(geckoService), usimEnabled);
  }

  test_table([0x08], "ADN", true, false);
  test_table([0x08], "FDN", false, false);
  test_table([0x08], "SDN", false, true);

  run_next_test();
});




add_test(function test_is_gsm_8bit_alphabet() {
  let worker = newUint8Worker();
  let ICCUtilsHelper = worker.ICCUtilsHelper;
  const langTable = PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
  const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];

  do_check_eq(ICCUtilsHelper.isGsm8BitAlphabet(langTable), true);
  do_check_eq(ICCUtilsHelper.isGsm8BitAlphabet(langShiftTable), true);
  do_check_eq(ICCUtilsHelper.isGsm8BitAlphabet("\uaaaa"), false);

  run_next_test();
});




add_test(function test_icc_get_card_lock_state_fdn() {
  let worker = newUint8Worker();
  let ril = worker.RIL;
  let buf = worker.Buf;

  buf.sendParcel = function () {
    
    do_check_eq(this.readInt32(), REQUEST_QUERY_FACILITY_LOCK)

    
    this.readInt32();

    
    do_check_eq(this.readInt32(), worker.RILQUIRKS_V5_LEGACY ? 3 : 4);

    
    do_check_eq(this.readString(), ICC_CB_FACILITY_FDN);

    
    do_check_eq(this.readString(), "");

    
    do_check_eq(this.readString(), (ICC_SERVICE_CLASS_VOICE |
                                    ICC_SERVICE_CLASS_DATA  |
                                    ICC_SERVICE_CLASS_FAX).toString());

    if (!worker.RILQUIRKS_V5_LEGACY) {
      
      this.readInt32();
    }

    run_next_test();
  };

  ril.iccGetCardLockState({lockType: "fdn"});
});

add_test(function test_get_network_name_from_icc() {
  let worker = newUint8Worker();
  let RIL = worker.RIL;
  let ICCUtilsHelper = worker.ICCUtilsHelper;

  function testGetNetworkNameFromICC(operatorData, expectedResult) {
    let result = ICCUtilsHelper.getNetworkNameFromICC(operatorData.mcc,
                                                      operatorData.mnc,
                                                      operatorData.lac);

    if (expectedResult == null) {
      do_check_eq(result, expectedResult);
    } else {
      do_check_eq(result.fullName, expectedResult.longName);
      do_check_eq(result.shortName, expectedResult.shortName);
    }
  }

  
  testGetNetworkNameFromICC({mcc: 123, mnc: 456, lac: 0x1000}, null);
  testGetNetworkNameFromICC({mcc: 321, mnc: 654, lac: 0x2000}, null);

  
  RIL.iccInfo.mcc = 123;
  RIL.iccInfo.mnc = 456;

  RIL.voiceRegistrationState = {
    cell: {
      gsmLocationAreaCode: 0x1000
    }
  };
  RIL.operator = {};

  
  RIL.iccInfoPrivate = {
    PNN: [
      {"fullName": "PNN1Long", "shortName": "PNN1Short"},
      {"fullName": "PNN2Long", "shortName": "PNN2Short"},
      {"fullName": "PNN3Long", "shortName": "PNN3Short"},
      {"fullName": "PNN4Long", "shortName": "PNN4Short"}
    ]
  };

  
  testGetNetworkNameFromICC({mcc: 321, mnc: 654, lac: 0x1000}, null);

  
  
  testGetNetworkNameFromICC({mcc: 123, mnc: 456, lac: 0x1000},
                            {longName: "PNN1Long", shortName: "PNN1Short"});

  
  RIL.iccInfoPrivate.OPL = [
    {
      "mcc": 123,
      "mnc": 456,
      "lacTacStart": 0,
      "lacTacEnd": 0xFFFE,
      "pnnRecordId": 4
    },
    {
      "mcc": 321,
      "mnc": 654,
      "lacTacStart": 0,
      "lacTacEnd": 0x0010,
      "pnnRecordId": 3
    },
    {
      "mcc": 321,
      "mnc": 654,
      "lacTacStart": 0x0100,
      "lacTacEnd": 0x1010,
      "pnnRecordId": 2
    }
  ];

  
  testGetNetworkNameFromICC({mcc: 123, mnc: 456, lac: 0x1000},
                            {longName: "PNN4Long", shortName: "PNN4Short"});

  
  
  testGetNetworkNameFromICC({mcc: 321, mnc: 654, lac: 0x1000},
                            {longName: "PNN2Long", shortName: "PNN2Short"});

  
  
  testGetNetworkNameFromICC({mcc: 321, mnc: 654, lac: 0x0001},
                            {longName: "PNN3Long", shortName: "PNN3Short"});

  run_next_test();
});

add_test(function test_path_id_for_spid_and_spn() {
  let worker = newWorker({
    postRILMessage: function fakePostRILMessage(data) {
      
    },
    postMessage: function fakePostMessage(message) {
      
    }});
  let RIL = worker.RIL;
  let ICCFileHelper = worker.ICCFileHelper;

  
  RIL.appType = CARD_APPTYPE_SIM;
  do_check_eq(ICCFileHelper.getEFPath(ICC_EF_SPDI),
              EF_PATH_MF_SIM + EF_PATH_DF_GSM);
  do_check_eq(ICCFileHelper.getEFPath(ICC_EF_SPN),
              EF_PATH_MF_SIM + EF_PATH_DF_GSM);

  
  RIL.appType = CARD_APPTYPE_USIM;
  do_check_eq(ICCFileHelper.getEFPath(ICC_EF_SPDI),
              EF_PATH_MF_SIM + EF_PATH_ADF_USIM);
  do_check_eq(ICCFileHelper.getEFPath(ICC_EF_SPDI),
              EF_PATH_MF_SIM + EF_PATH_ADF_USIM);
  run_next_test();
});




add_test(function test_parse_pbr_tlvs() {
  let worker = newUint8Worker();
  let buf = worker.Buf;
  let pduHelper = worker.GsmPDUHelper;

  let pbrTlvs = [
    {tag: ICC_USIM_TYPE1_TAG,
     length: 0x0F,
     value: [{tag: ICC_USIM_EFADN_TAG,
              length: 0x03,
              value: [0x4F, 0x3A, 0x02]},
             {tag: ICC_USIM_EFIAP_TAG,
              length: 0x03,
              value: [0x4F, 0x25, 0x01]},
             {tag: ICC_USIM_EFPBC_TAG,
              length: 0x03,
              value: [0x4F, 0x09, 0x04]}]
    },
    {tag: ICC_USIM_TYPE2_TAG,
     length: 0x05,
     value: [{tag: ICC_USIM_EFEMAIL_TAG,
              length: 0x03,
              value: [0x4F, 0x50, 0x0B]},
             {tag: ICC_USIM_EFANR_TAG,
              length: 0x03,
              value: [0x4F, 0x11, 0x02]},
             {tag: ICC_USIM_EFANR_TAG,
              length: 0x03,
              value: [0x4F, 0x12, 0x03]}]
    },
    {tag: ICC_USIM_TYPE3_TAG,
     length: 0x0A,
     value: [{tag: ICC_USIM_EFCCP1_TAG,
              length: 0x03,
              value: [0x4F, 0x3D, 0x0A]},
             {tag: ICC_USIM_EFEXT1_TAG,
              length: 0x03,
              value: [0x4F, 0x4A, 0x03]}]
    },
  ];

  let pbr = worker.ICCUtilsHelper.parsePbrTlvs(pbrTlvs);
  do_check_eq(pbr.adn.fileId, 0x4F3a);
  do_check_eq(pbr.iap.fileId, 0x4F25);
  do_check_eq(pbr.pbc.fileId, 0x4F09);
  do_check_eq(pbr.email.fileId, 0x4F50);
  do_check_eq(pbr.anr0.fileId, 0x4f11);
  do_check_eq(pbr.anr1.fileId, 0x4f12);
  do_check_eq(pbr.ccp1.fileId, 0x4F3D);
  do_check_eq(pbr.ext1.fileId, 0x4F4A);

  run_next_test();
});




add_test(function test_load_linear_fixed_ef() {
  let worker = newUint8Worker();
  let ril = worker.RIL;
  let io = worker.ICCIOHelper;

  io.getResponse = function fakeGetResponse(options) {
    
    do_check_true(false);
    run_next_test();
  };

  ril.iccIO = function fakeIccIO(options) {
    do_check_true(true);
    run_next_test();
  };

  io.loadLinearFixedEF({recordSize: 0x20});
});




add_test(function test_load_linear_fixed_ef() {
  let worker = newUint8Worker();
  let ril = worker.RIL;
  let io = worker.ICCIOHelper;

  io.getResponse = function fakeGetResponse(options) {
    do_check_true(true);
    run_next_test();
  };

  ril.iccIO = function fakeIccIO(options) {
    
    do_check_true(false);
    run_next_test();
  };

  io.loadLinearFixedEF({});
});




add_test(function test_read_pbr() {
  let worker = newUint8Worker();
  let helper = worker.GsmPDUHelper;
  let record = worker.ICCRecordHelper;
  let buf    = worker.Buf;
  let io     = worker.ICCIOHelper;

  io.loadLinearFixedEF = function fakeLoadLinearFixedEF(options)  {
    let pbr_1 = [
      0xa8, 0x05, 0xc0, 0x03, 0x4f, 0x3a, 0x01
    ];

    
    buf.writeInt32(pbr_1.length * 2);

    
    for (let i = 0; i < pbr_1.length; i++) {
      helper.writeHexOctet(pbr_1[i]);
    }

    
    buf.writeStringDelimiter(pbr_1.length * 2);

    options.totalRecords = 2;
    if (options.callback) {
      options.callback(options);
    }
  };

  io.loadNextRecord = function fakeLoadNextRecord(options) {
    let pbr_2 = [
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    ];

    options.p1++;
    if (options.callback) {
      options.callback(options);
    }
  };

  let successCb = function successCb(pbrs) {
    do_check_eq(pbrs[0].adn.fileId, 0x4f3a);
    do_check_eq(pbrs.length, 1);
    run_next_test();
  };

  let errorCb = function errorCb(errorMsg) {
    do_print("Reading EF_PBR failed, msg = " + errorMsg);
    do_check_true(false);
    run_next_test();
  };

  record.readPBR(successCb, errorCb);
});




add_test(function test_read_email() {
  let worker = newUint8Worker();
  let helper = worker.GsmPDUHelper;
  let record = worker.ICCRecordHelper;
  let buf    = worker.Buf;
  let io     = worker.ICCIOHelper;
  let recordSize;

  io.loadLinearFixedEF = function fakeLoadLinearFixedEF(options)  {
    let email_1 = [
      0x65, 0x6D, 0x61, 0x69, 0x6C,
      0x00, 0x6D, 0x6F, 0x7A, 0x69,
      0x6C, 0x6C, 0x61, 0x2E, 0x63,
      0x6F, 0x6D, 0x02, 0x23];

    
    buf.writeInt32(email_1.length * 2);

    
    for (let i = 0; i < email_1.length; i++) {
      helper.writeHexOctet(email_1[i]);
    }

    
    buf.writeStringDelimiter(email_1.length * 2);

    recordSize = email_1.length;
    options.recordSize = recordSize;
    if (options.callback) {
      options.callback(options);
    }
  };

  function doTestReadEmail(type, expectedResult) {
    let fileId = 0x6a75;
    let recordNumber = 1;

    
    record.readEmail(fileId, type, recordNumber, function (email) {
      do_check_eq(email, expectedResult);
    });
  };

  doTestReadEmail(ICC_USIM_TYPE1_TAG, "email@mozilla.com$#");
  doTestReadEmail(ICC_USIM_TYPE2_TAG, "email@mozilla.com");
  do_check_eq(record._emailRecordSize, recordSize);

  run_next_test();
});




add_test(function test_update_email() {
  const recordSize = 0x20;
  const recordNumber = 1;
  const fileId = 0x4f50;
  const NUM_TESTS = 2;
  let worker = newUint8Worker();
  let pduHelper = worker.GsmPDUHelper;
  let ril = worker.RIL;
  ril.appType = CARD_APPTYPE_USIM;
  let recordHelper = worker.ICCRecordHelper;
  let buf = worker.Buf;
  let ioHelper = worker.ICCIOHelper;
  let pbr = {email: {fileId: fileId, fileType: ICC_USIM_TYPE1_TAG},
             adn: {sfi: 1}};
  let count = 0;

  
  ioHelper.updateLinearFixedEF = function (options) {
    options.pathId = worker.ICCFileHelper.getEFPath(options.fileId);
    options.command = ICC_COMMAND_UPDATE_RECORD;
    options.p1 = options.recordNumber;
    options.p2 = READ_RECORD_ABSOLUTE_MODE;
    options.p3 = recordSize;
    ril.iccIO(options);
  };

  function do_test(pbr, expectedEmail, expectedAdnRecordId) {
    buf.sendParcel = function () {
      count++;

      
      do_check_eq(this.readInt32(), REQUEST_SIM_IO);

      
      this.readInt32();

      
      do_check_eq(this.readInt32(), ICC_COMMAND_UPDATE_RECORD);

      
      do_check_eq(this.readInt32(), fileId);

      
      do_check_eq(this.readString(),
                  EF_PATH_MF_SIM + EF_PATH_DF_TELECOM + EF_PATH_DF_PHONEBOOK);

      
      do_check_eq(this.readInt32(), recordNumber);

      
      do_check_eq(this.readInt32(), READ_RECORD_ABSOLUTE_MODE);

      
      do_check_eq(this.readInt32(), recordSize);

      
      let strLen = this.readInt32();
      let email;
      if (pbr.email.fileType === ICC_USIM_TYPE1_TAG) {
        email = pduHelper.read8BitUnpackedToString(recordSize);
      } else {
        email = pduHelper.read8BitUnpackedToString(recordSize - 2);
        do_check_eq(pduHelper.readHexOctet(), pbr.adn.sfi);
        do_check_eq(pduHelper.readHexOctet(), expectedAdnRecordId);
      }
      this.readStringDelimiter(strLen);
      do_check_eq(email, expectedEmail);

      
      do_check_eq(this.readString(), null);

      if (!worker.RILQUIRKS_V5_LEGACY) {
        
        this.readInt32();
      }

      if (count == NUM_TESTS) {
        run_next_test();
      }
    };
    recordHelper.updateEmail(pbr, recordNumber, expectedEmail, expectedAdnRecordId);
  }

  do_test(pbr, "test@mail.com");
  pbr.email.fileType = ICC_USIM_TYPE2_TAG;
  do_test(pbr, "test@mail.com", 1);
});




add_test(function test_read_anr() {
  let worker = newUint8Worker();
  let helper = worker.GsmPDUHelper;
  let record = worker.ICCRecordHelper;
  let buf    = worker.Buf;
  let io     = worker.ICCIOHelper;
  let recordSize;

  io.loadLinearFixedEF = function fakeLoadLinearFixedEF(options)  {
    let anr_1 = [
      0x01, 0x05, 0x81, 0x10, 0x32,
      0x54, 0xF6, 0xFF, 0xFF];

    
    buf.writeInt32(anr_1.length * 2);

    
    for (let i = 0; i < anr_1.length; i++) {
      helper.writeHexOctet(anr_1[i]);
    }

    
    buf.writeStringDelimiter(anr_1.length * 2);

    recordSize = anr_1.length;
    options.recordSize = recordSize;
    if (options.callback) {
      options.callback(options);
    }
  };

  function doTestReadAnr(fileType, expectedResult) {
    let fileId = 0x4f11;
    let recordNumber = 1;

    
    record.readANR(fileId, fileType, recordNumber, function (anr) {
      do_check_eq(anr, expectedResult);
    });
  };

  doTestReadAnr(ICC_USIM_TYPE1_TAG, "0123456");
  do_check_eq(record._anrRecordSize, recordSize);

  run_next_test();
});




add_test(function test_update_anr() {
  const recordSize = 0x20;
  const recordNumber = 1;
  const fileId = 0x4f11;
  const NUM_TESTS = 2;
  let worker = newUint8Worker();
  let pduHelper = worker.GsmPDUHelper;
  let ril = worker.RIL;
  ril.appType = CARD_APPTYPE_USIM;
  let recordHelper = worker.ICCRecordHelper;
  let buf = worker.Buf;
  let ioHelper = worker.ICCIOHelper;
  let pbr = {anr0: {fileId: fileId, fileType: ICC_USIM_TYPE1_TAG},
             adn: {sfi: 1}};
  let count = 0;

  
  ioHelper.updateLinearFixedEF = function (options) {
    options.pathId = worker.ICCFileHelper.getEFPath(options.fileId);
    options.command = ICC_COMMAND_UPDATE_RECORD;
    options.p1 = options.recordNumber;
    options.p2 = READ_RECORD_ABSOLUTE_MODE;
    options.p3 = recordSize;
    ril.iccIO(options);
  };

  function do_test(pbr, expectedANR, expectedAdnRecordId) {
    buf.sendParcel = function () {
      count++;

      
      do_check_eq(this.readInt32(), REQUEST_SIM_IO);

      
      this.readInt32();

      
      do_check_eq(this.readInt32(), ICC_COMMAND_UPDATE_RECORD);

      
      do_check_eq(this.readInt32(), fileId);

      
      do_check_eq(this.readString(),
                  EF_PATH_MF_SIM + EF_PATH_DF_TELECOM + EF_PATH_DF_PHONEBOOK);

      
      do_check_eq(this.readInt32(), recordNumber);

      
      do_check_eq(this.readInt32(), READ_RECORD_ABSOLUTE_MODE);

      
      do_check_eq(this.readInt32(), recordSize);

      
      let strLen = this.readInt32();
      
      pduHelper.readHexOctet();
      do_check_eq(pduHelper.readNumberWithLength(), expectedANR);
      
      pduHelper.readHexOctet();
      
      pduHelper.readHexOctet();
      if (pbr.anr0.fileType === ICC_USIM_TYPE2_TAG) {
        do_check_eq(pduHelper.readHexOctet(), pbr.adn.sfi);
        do_check_eq(pduHelper.readHexOctet(), expectedAdnRecordId);
      }
      this.readStringDelimiter(strLen);

      
      do_check_eq(this.readString(), null);

      if (!worker.RILQUIRKS_V5_LEGACY) {
        
        this.readInt32();
      }

      if (count == NUM_TESTS) {
        run_next_test();
      }
    };
    recordHelper.updateANR(pbr, recordNumber, expectedANR, expectedAdnRecordId);
  }

  do_test(pbr, "+123456789");
  pbr.anr0.fileType = ICC_USIM_TYPE2_TAG;
  do_test(pbr, "123456789", 1);
});




add_test(function test_read_iap() {
  let worker = newUint8Worker();
  let helper = worker.GsmPDUHelper;
  let record = worker.ICCRecordHelper;
  let buf    = worker.Buf;
  let io     = worker.ICCIOHelper;
  let recordSize;

  io.loadLinearFixedEF = function fakeLoadLinearFixedEF(options)  {
    let iap_1 = [0x01, 0x02];

    
    buf.writeInt32(iap_1.length * 2);

    
    for (let i = 0; i < iap_1.length; i++) {
      helper.writeHexOctet(iap_1[i]);
    }

    
    buf.writeStringDelimiter(iap_1.length * 2);

    recordSize = iap_1.length;
    options.recordSize = recordSize;
    if (options.callback) {
      options.callback(options);
    }
  };

  function doTestReadIAP(expectedIAP) {
    const fileId = 0x4f17;
    const recordNumber = 1;

    let successCb = function successCb(iap) {
      for (let i = 0; i < iap.length; i++) {
        do_check_eq(expectedIAP[i], iap[i]);
      }
      run_next_test();
    }.bind(this);

    let errorCb = function errorCb(errorMsg) {
      do_print(errorMsg);
      do_check_true(false);
      run_next_test();
    }.bind(this);

    record.readIAP(fileId, recordNumber, successCb, errorCb);
  };

  doTestReadIAP([1, 2]);
});




add_test(function test_update_iap() {
  const recordSize = 2;
  const recordNumber = 1;
  const fileId = 0x4f17;
  let worker = newUint8Worker();
  let pduHelper = worker.GsmPDUHelper;
  let ril = worker.RIL;
  ril.appType = CARD_APPTYPE_USIM;
  let recordHelper = worker.ICCRecordHelper;
  let buf = worker.Buf;
  let ioHelper = worker.ICCIOHelper;
  let count = 0;

  
  ioHelper.updateLinearFixedEF = function (options) {
    options.pathId = worker.ICCFileHelper.getEFPath(options.fileId);
    options.command = ICC_COMMAND_UPDATE_RECORD;
    options.p1 = options.recordNumber;
    options.p2 = READ_RECORD_ABSOLUTE_MODE;
    options.p3 = recordSize;
    ril.iccIO(options);
  };

  function do_test(expectedIAP) {
    buf.sendParcel = function () {
      
      do_check_eq(this.readInt32(), REQUEST_SIM_IO);

      
      this.readInt32();

      
      do_check_eq(this.readInt32(), ICC_COMMAND_UPDATE_RECORD);

      
      do_check_eq(this.readInt32(), fileId);

      
      do_check_eq(this.readString(),
                  EF_PATH_MF_SIM + EF_PATH_DF_TELECOM + EF_PATH_DF_PHONEBOOK);

      
      do_check_eq(this.readInt32(), recordNumber);

      
      do_check_eq(this.readInt32(), READ_RECORD_ABSOLUTE_MODE);

      
      do_check_eq(this.readInt32(), recordSize);

      
      let strLen = this.readInt32();
      for (let i = 0; i < recordSize; i++) {
        do_check_eq(expectedIAP[i], pduHelper.readHexOctet());
      }
      this.readStringDelimiter(strLen);

      
      do_check_eq(this.readString(), null);

      if (!worker.RILQUIRKS_V5_LEGACY) {
        
        this.readInt32();
      }

      run_next_test();
    };
    recordHelper.updateIAP(fileId, recordNumber, expectedIAP);
  }

  do_test([1, 2]);
});




add_test(function test_update_adn_like() {
  let worker = newUint8Worker();
  let ril = worker.RIL;
  let record = worker.ICCRecordHelper;
  let io = worker.ICCIOHelper;
  let pdu = worker.GsmPDUHelper;
  let buf = worker.Buf;

  ril.appType = CARD_APPTYPE_SIM;
  const recordSize = 0x20;
  let fileId;

  
  io.updateLinearFixedEF = function (options) {
    options.pathId = worker.ICCFileHelper.getEFPath(options.fileId);
    options.command = ICC_COMMAND_UPDATE_RECORD;
    options.p1 = options.recordNumber;
    options.p2 = READ_RECORD_ABSOLUTE_MODE;
    options.p3 = recordSize;
    ril.iccIO(options);
  };

  buf.sendParcel = function () {
    
    do_check_eq(this.readInt32(), REQUEST_SIM_IO);

    
    this.readInt32();

    
    do_check_eq(this.readInt32(), ICC_COMMAND_UPDATE_RECORD);

    
    do_check_eq(this.readInt32(), fileId);

    
    do_check_eq(this.readString(), EF_PATH_MF_SIM + EF_PATH_DF_TELECOM);

    
    do_check_eq(this.readInt32(), 1);

    
    do_check_eq(this.readInt32(), READ_RECORD_ABSOLUTE_MODE);

    
    do_check_eq(this.readInt32(), 0x20);

    
    let contact = pdu.readAlphaIdDiallingNumber(0x20);
    do_check_eq(contact.alphaId, "test");
    do_check_eq(contact.number, "123456");

    
    if (fileId == ICC_EF_ADN) {
      do_check_eq(this.readString(), null);
    } else {
      do_check_eq(this.readString(), "1111");
    }

    if (!worker.RILQUIRKS_V5_LEGACY) {
      
      this.readInt32();
    }

    if (fileId == ICC_EF_FDN) {
      run_next_test();
    }
  };

  fileId = ICC_EF_ADN;
  record.updateADNLike(fileId,
                       {recordId: 1, alphaId: "test", number: "123456"});

  fileId = ICC_EF_FDN;
  record.updateADNLike(fileId,
                       {recordId: 1, alphaId: "test", number: "123456"},
                       "1111");
});




add_test(function test_find_free_record_id() {
  let worker = newUint8Worker();
  let pduHelper = worker.GsmPDUHelper;
  let recordHelper = worker.ICCRecordHelper;
  let buf = worker.Buf;
  let io  = worker.ICCIOHelper;

  function writeRecord (record) {
    
    buf.writeInt32(record.length * 2);

    for (let i = 0; i < record.length; i++) {
      pduHelper.writeHexOctet(record[i]);
    }

    
    buf.writeStringDelimiter(record.length * 2);
  }

  io.loadLinearFixedEF = function fakeLoadLinearFixedEF(options)  {
    
    let record = [0x12, 0x34, 0x56, 0x78, 0x90];
    options.p1 = 1;
    options.totalRecords = 2;
    writeRecord(record);
    if (options.callback) {
      options.callback(options);
    }
  };

  io.loadNextRecord = function fakeLoadNextRecord(options) {
    
    let record = [0xff, 0xff, 0xff, 0xff, 0xff];
    options.p1++;
    writeRecord(record);
    if (options.callback) {
      options.callback(options);
    }
  };

  let fileId = 0x0000; 
  recordHelper.findFreeRecordId(
    fileId,
    function (recordId) {
      do_check_eq(recordId, 2);
      run_next_test();
    }.bind(this),
    function (errorMsg) {
      do_print(errorMsg);
      do_check_true(false);
      run_next_test();
    }.bind(this));
});




add_test(function test_read_icc_contacts() {
  let worker = newUint8Worker();
  let record = worker.ICCRecordHelper;
  let contactHelper = worker.ICCContactHelper;

  function do_test(aSimType, aContactType, aExpectedContact, aEnhancedPhoneBook) {
    worker.RIL.appType = aSimType;
    worker.RIL._isCdma = (aSimType === CARD_APPTYPE_RUIM);
    worker.RIL.iccInfoPrivate.cst = (aEnhancedPhoneBook) ?
                                    [0x0, 0x0C, 0x0, 0x0, 0x0]:
                                    [0x0, 0x00, 0x0, 0x0, 0x0];

    
    contactHelper.getContactFieldRecordId = function (pbr, contact, field, onsuccess, onerror) {
      onsuccess(1);
    };

    record.readPBR = function readPBR(onsuccess, onerror) {
      onsuccess([{adn:{fileId: 0x6f3a}, email: {}, anr0: {}}]);
    };

    record.readADNLike = function readADNLike(fileId, onsuccess, onerror) {
      onsuccess([{recordId: 1, alphaId: "name", number: "111111"}])
    };

    record.readEmail = function readEmail(fileId, fileType, recordNumber, onsuccess, onerror) {
      onsuccess("hello@mail.com");
    };

    record.readANR = function readANR(fileId, fileType, recordNumber, onsuccess, onerror) {
      onsuccess("123456");
    };

    let onsuccess = function onsuccess(contacts) {
      let contact = contacts[0];
      for (let key in contact) {
        do_print("check " + key);
        if (Array.isArray(contact[key])) {
          do_check_eq(contact[key][0], aExpectedContact[key]);
        } else {
          do_check_eq(contact[key], aExpectedContact[key]);
        }
      }
    };

    let onerror = function onerror(errorMsg) {
      do_print("readICCContacts failed: " + errorMsg);
      do_check_true(false);
    };

    contactHelper.readICCContacts(aSimType, aContactType, onsuccess, onerror);
  }

  let expectedContact1 = {
    pbrIndex: 0,
    recordId: 1,
    alphaId:  "name",
    number:   "111111"
  };

  let expectedContact2 = {
    pbrIndex: 0,
    recordId: 1,
    alphaId:  "name",
    number:   "111111",
    email:    "hello@mail.com",
    anr:      "123456"
  };

  
  do_print("Test read SIM adn contacts");
  do_test(CARD_APPTYPE_SIM, "adn", expectedContact1);

  do_print("Test read SIM fdn contacts");
  do_test(CARD_APPTYPE_SIM, "fdn", expectedContact1);

  
  do_print("Test read USIM adn contacts");
  do_test(CARD_APPTYPE_USIM, "adn", expectedContact2);

  do_print("Test read USIM fdn contacts");
  do_test(CARD_APPTYPE_USIM, "fdn", expectedContact1);

  
  do_print("Test read RUIM adn contacts");
  do_test(CARD_APPTYPE_RUIM, "adn", expectedContact1);

  do_print("Test read RUIM fdn contacts");
  do_test(CARD_APPTYPE_RUIM, "fdn", expectedContact1);

  
  do_print("Test read RUIM adn contacts with enhanced phone book");
  do_test(CARD_APPTYPE_RUIM, "adn", expectedContact2, true);

  do_print("Test read RUIM fdn contacts with enhanced phone book");
  do_test(CARD_APPTYPE_RUIM, "fdn", expectedContact1, true);

  run_next_test();
});




add_test(function test_update_icc_contact() {
  const ADN_RECORD_ID   = 100;
  const ADN_SFI         = 1;
  const IAP_FILE_ID     = 0x4f17;
  const EMAIL_FILE_ID   = 0x4f50;
  const EMAIL_RECORD_ID = 20;
  const ANR0_FILE_ID    = 0x4f11;
  const ANR0_RECORD_ID  = 30;

  let worker = newUint8Worker();
  let recordHelper = worker.ICCRecordHelper;
  let contactHelper = worker.ICCContactHelper;

  function do_test(aSimType, aContactType, aContact, aPin2, aFileType, aEnhancedPhoneBook) {
    worker.RIL.appType = aSimType;
    worker.RIL._isCdma = (aSimType === CARD_APPTYPE_RUIM);
    worker.RIL.iccInfoPrivate.cst = (aEnhancedPhoneBook) ?
                                    [0x0, 0x0C, 0x0, 0x0, 0x0]:
                                    [0x0, 0x00, 0x0, 0x0, 0x0];

    recordHelper.readPBR = function (onsuccess, onerror) {
      if (aFileType === ICC_USIM_TYPE1_TAG) {
        onsuccess([{
          adn:   {fileId: ICC_EF_ADN},
          email: {fileId: EMAIL_FILE_ID,
                  fileType: ICC_USIM_TYPE1_TAG},
          anr0:  {fileId: ANR0_FILE_ID,
                  fileType: ICC_USIM_TYPE1_TAG}
        }]);
      } else if (aFileType === ICC_USIM_TYPE2_TAG) {
        onsuccess([{
          adn:   {fileId: ICC_EF_ADN,
                  sfi: ADN_SFI},
          iap:   {fileId: IAP_FILE_ID},
          email: {fileId: EMAIL_FILE_ID,
                  fileType: ICC_USIM_TYPE2_TAG,
                  indexInIAP: 0},
          anr0:  {fileId: ANR0_FILE_ID,
                  fileType: ICC_USIM_TYPE2_TAG,
                  indexInIAP: 1}
        }]);
      }
    };

    recordHelper.updateADNLike = function (fileId, contact, pin2, onsuccess, onerror) {
      if (aContactType === "fdn") {
        do_check_eq(fileId, ICC_EF_FDN);
      } else if (aContactType === "adn") {
        do_check_eq(fileId, ICC_EF_ADN);
      }
      do_check_eq(pin2, aPin2);
      do_check_eq(contact.alphaId, aContact.alphaId);
      do_check_eq(contact.number, aContact.number);
      onsuccess();
    };

    recordHelper.readIAP = function (fileId, recordNumber, onsuccess, onerror) {
      do_check_eq(fileId, IAP_FILE_ID);
      do_check_eq(recordNumber, ADN_RECORD_ID);
      onsuccess([EMAIL_RECORD_ID, ANR0_RECORD_ID]);
    };

    recordHelper.updateEmail = function (pbr, recordNumber, email, adnRecordId, onsuccess, onerror) {
      do_check_eq(pbr.email.fileId, EMAIL_FILE_ID);
      if (pbr.email.fileType === ICC_USIM_TYPE1_TAG) {
        do_check_eq(recordNumber, ADN_RECORD_ID);
      } else if (pbr.email.fileType === ICC_USIM_TYPE2_TAG) {
        do_check_eq(recordNumber, EMAIL_RECORD_ID);
      }
      do_check_eq(email, aContact.email);
      onsuccess();
    };

    recordHelper.updateANR = function (pbr, recordNumber, number, adnRecordId, onsuccess, onerror) {
      do_check_eq(pbr.anr0.fileId, ANR0_FILE_ID);
      if (pbr.anr0.fileType === ICC_USIM_TYPE1_TAG) {
        do_check_eq(recordNumber, ADN_RECORD_ID);
      } else if (pbr.anr0.fileType === ICC_USIM_TYPE2_TAG) {
        do_check_eq(recordNumber, ANR0_RECORD_ID);
      }
      do_check_eq(number, aContact.anr[0]);
      onsuccess();
    };

    let onsuccess = function onsuccess() {
      do_print("updateICCContact success");
    };

    let onerror = function onerror(errorMsg) {
      do_print("updateICCContact failed: " + errorMsg);
      do_check_true(false);
    };

    contactHelper.updateICCContact(aSimType, aContactType, aContact, aPin2, onsuccess, onerror);
  }

  let contacts = [
    {
      pbrIndex: 0,
      recordId: ADN_RECORD_ID,
      alphaId:  "test",
      number:   "123456",
      email:    "test@mail.com",
      anr:      ["+654321"]
    },
    
    {
      pbrIndex: 0,
      recordId: ADN_RECORD_ID,
      alphaId:  "test2",
      number:   "123456",
    }];

  for (let i = 0; i < contacts.length; i++) {
    let contact = contacts[i];
    
    do_print("Test update SIM adn contacts");
    do_test(CARD_APPTYPE_SIM, "adn", contact);

    do_print("Test update SIM fdn contacts");
    do_test(CARD_APPTYPE_SIM, "fdn", contact, "1234");

    
    do_print("Test update USIM adn contacts");
    do_test(CARD_APPTYPE_USIM, "adn", contact, null, ICC_USIM_TYPE1_TAG);
    do_test(CARD_APPTYPE_USIM, "adn", contact, null, ICC_USIM_TYPE2_TAG);

    do_print("Test update USIM fdn contacts");
    do_test(CARD_APPTYPE_USIM, "fdn", contact, "1234");

    
    do_print("Test update RUIM adn contacts");
    do_test(CARD_APPTYPE_RUIM, "adn", contact);

    do_print("Test update RUIM fdn contacts");
    do_test(CARD_APPTYPE_RUIM, "fdn", contact, "1234");

    
    do_print("Test update RUIM adn contacts with enhanced phone book");
    do_test(CARD_APPTYPE_RUIM, "adn", contact, null, ICC_USIM_TYPE1_TAG, true);
    do_test(CARD_APPTYPE_RUIM, "adn", contact, null, ICC_USIM_TYPE2_TAG, true);

    do_print("Test update RUIM fdn contacts with enhanced phone book");
    do_test(CARD_APPTYPE_RUIM, "fdn", contact, "1234", null, true);
  }

  run_next_test();
});




add_test(function test_find_free_icc_contact_sim() {
  let worker = newUint8Worker();
  let recordHelper = worker.ICCRecordHelper;
  let contactHelper = worker.ICCContactHelper;
  
  let records = [null];
  const MAX_RECORDS = 3;
  const PBR_INDEX = 0;

  recordHelper.findFreeRecordId = function (fileId, onsuccess, onerror) {
    if (records.length > MAX_RECORDS) {
      onerror("No free record found.");
      return;
    }

    onsuccess(records.length);
  };

  let successCb = function (pbrIndex, recordId) {
    do_check_eq(pbrIndex, PBR_INDEX);
    records[recordId] = {};
  };

  let errorCb = function (errorMsg) {
    do_print(errorMsg);
    do_check_true(false);
  };

  for (let i = 0; i < MAX_RECORDS; i++) {
    contactHelper.findFreeICCContact(CARD_APPTYPE_SIM, "adn", successCb, errorCb);
  }
  
  do_check_eq(records.length - 1, MAX_RECORDS);

  
  successCb = function (pbrIndex, recordId) {
    do_check_true(false);
  };

  errorCb = function (errorMsg) {
    do_check_true(errorMsg === "No free record found.");
  };
  contactHelper.findFreeICCContact(CARD_APPTYPE_SIM, "adn", successCb, errorCb);

  run_next_test();
});




add_test(function test_find_free_icc_contact_usim() {
  let worker = newUint8Worker();
  let recordHelper = worker.ICCRecordHelper;
  let contactHelper = worker.ICCContactHelper;
  const ADN1_FILE_ID = 0x6f3a;
  const ADN2_FILE_ID = 0x6f3b;
  const MAX_RECORDS = 3;

  
  
  let pbrs = [{adn: {fileId: ADN1_FILE_ID, records: [null, {}, {}]}},
              {adn: {fileId: ADN2_FILE_ID, records: [null]}}];

  recordHelper.readPBR = function readPBR(onsuccess, onerror) {
    onsuccess(pbrs);
  };

  recordHelper.findFreeRecordId = function (fileId, onsuccess, onerror) {
    let pbr = (fileId == ADN1_FILE_ID ? pbrs[0]: pbrs[1]);
    if (pbr.adn.records.length > MAX_RECORDS) {
      onerror("No free record found.");
      return;
    }

    onsuccess(pbr.adn.records.length);
  };

  let successCb = function (pbrIndex, recordId) {
    do_check_eq(pbrIndex, 0);
    pbrs[pbrIndex].adn.records[recordId] = {};
  };

  let errorCb = function (errorMsg) {
    do_check_true(false);
  };

  contactHelper.findFreeICCContact(CARD_APPTYPE_USIM, "adn", successCb, errorCb);

  
  
  successCb = function (pbrIndex, recordId) {
    do_check_eq(pbrIndex, 1);
    do_check_eq(recordId, 1);
  }
  contactHelper.findFreeICCContact(CARD_APPTYPE_USIM, "adn", successCb, errorCb);

  run_next_test();
});

add_test(function test_personalization_state() {
  let worker = newUint8Worker();
  let ril = worker.RIL;

  worker.ICCRecordHelper.readICCID = function fakeReadICCID() {};

  function testPersonalization(cardPersoState, geckoCardState) {
    let iccStatus = {
      cardState: CARD_STATE_PRESENT,
      gsmUmtsSubscriptionAppIndex: 0,
      apps: [
        {
          app_state: CARD_APPSTATE_SUBSCRIPTION_PERSO,
          perso_substate: cardPersoState
        }],
    };

    ril._processICCStatus(iccStatus);
    do_check_eq(ril.cardState, geckoCardState);
  }

  testPersonalization(CARD_PERSOSUBSTATE_SIM_NETWORK,
                      GECKO_CARDSTATE_NETWORK_LOCKED);
  testPersonalization(CARD_PERSOSUBSTATE_SIM_CORPORATE,
                      GECKO_CARDSTATE_CORPORATE_LOCKED);
  testPersonalization(CARD_PERSOSUBSTATE_SIM_SERVICE_PROVIDER,
                      GECKO_CARDSTATE_SERVICE_PROVIDER_LOCKED);
  testPersonalization(CARD_PERSOSUBSTATE_SIM_NETWORK_PUK,
                      GECKO_CARDSTATE_NETWORK_PUK_REQUIRED);
  testPersonalization(CARD_PERSOSUBSTATE_SIM_CORPORATE_PUK,
                      GECKO_CARDSTATE_CORPORATE_PUK_REQUIRED);
  testPersonalization(CARD_PERSOSUBSTATE_SIM_SERVICE_PROVIDER_PUK,
                      GECKO_CARDSTATE_SERVICE_PROVIDER_PUK_REQUIRED);
  testPersonalization(CARD_PERSOSUBSTATE_READY,
                      GECKO_CARDSTATE_PERSONALIZATION_READY);

  run_next_test();
});




add_test(function test_card_app_state() {
  let worker = newUint8Worker();
  let ril = worker.RIL;

  worker.ICCRecordHelper.readICCID = function fakeReadICCID() {};

  function testCardAppState(cardAppState, geckoCardState) {
    let iccStatus = {
      cardState: CARD_STATE_PRESENT,
      gsmUmtsSubscriptionAppIndex: 0,
      apps: [
      {
        app_state: cardAppState
      }],
    };

    ril._processICCStatus(iccStatus);
    do_check_eq(ril.cardState, geckoCardState);
  }

  testCardAppState(CARD_APPSTATE_ILLEGAL,
                   GECKO_CARDSTATE_ILLEGAL);
  testCardAppState(CARD_APPSTATE_PIN,
                   GECKO_CARDSTATE_PIN_REQUIRED);
  testCardAppState(CARD_APPSTATE_PUK,
                   GECKO_CARDSTATE_PUK_REQUIRED);
  testCardAppState(CARD_APPSTATE_READY,
                   GECKO_CARDSTATE_READY);
  testCardAppState(CARD_APPSTATE_UNKNOWN,
                   GECKO_CARDSTATE_UNKNOWN);
  testCardAppState(CARD_APPSTATE_DETECTED,
                   GECKO_CARDSTATE_UNKNOWN);

  run_next_test();
});




add_test(function test_icc_permanent_blocked() {
  let worker = newUint8Worker();
  let ril = worker.RIL;

  worker.ICCRecordHelper.readICCID = function fakeReadICCID() {};

  function testPermanentBlocked(pin1_replaced, universalPINState, pin1) {
    let iccStatus = {
      cardState: CARD_STATE_PRESENT,
      gsmUmtsSubscriptionAppIndex: 0,
      universalPINState: universalPINState,
      apps: [
      {
        pin1_replaced: pin1_replaced,
        pin1: pin1
      }]
    };

    ril._processICCStatus(iccStatus);
    do_check_eq(ril.cardState, GECKO_CARDSTATE_PERMANENT_BLOCKED);
  }

  testPermanentBlocked(1,
                       CARD_PINSTATE_ENABLED_PERM_BLOCKED,
                       CARD_PINSTATE_UNKNOWN);
  testPermanentBlocked(1,
                       CARD_PINSTATE_ENABLED_PERM_BLOCKED,
                       CARD_PINSTATE_ENABLED_PERM_BLOCKED);
  testPermanentBlocked(0,
                       CARD_PINSTATE_UNKNOWN,
                       CARD_PINSTATE_ENABLED_PERM_BLOCKED);

  run_next_test();
});




add_test(function test_set_icc_card_lock_facility_lock() {
  let worker = newUint8Worker();
  worker.RILQUIRKS_V5_LEGACY = false;
  let aid = "123456789";
  let ril = worker.RIL;
  ril.aid = aid;
  let buf = worker.Buf;

  let GECKO_CARDLOCK_TO_FACILITIY_LOCK = {};
  GECKO_CARDLOCK_TO_FACILITIY_LOCK[GECKO_CARDLOCK_PIN] = ICC_CB_FACILITY_SIM;
  GECKO_CARDLOCK_TO_FACILITIY_LOCK[GECKO_CARDLOCK_FDN] = ICC_CB_FACILITY_FDN;

  let GECKO_CARDLOCK_TO_PASSWORD_TYPE = {};
  GECKO_CARDLOCK_TO_PASSWORD_TYPE[GECKO_CARDLOCK_PIN] = "pin";
  GECKO_CARDLOCK_TO_PASSWORD_TYPE[GECKO_CARDLOCK_FDN] = "pin2";

  const pin = "1234";
  const pin2 = "4321";
  let GECKO_CARDLOCK_TO_PASSWORD = {};
  GECKO_CARDLOCK_TO_PASSWORD[GECKO_CARDLOCK_PIN] = pin;
  GECKO_CARDLOCK_TO_PASSWORD[GECKO_CARDLOCK_FDN] = pin2;

  const serviceClass = ICC_SERVICE_CLASS_VOICE |
                       ICC_SERVICE_CLASS_DATA  |
                       ICC_SERVICE_CLASS_FAX;

  function do_test(aLock, aPassword, aEnabled) {
    buf.sendParcel = function fakeSendParcel () {
      
      do_check_eq(this.readInt32(), REQUEST_SET_FACILITY_LOCK);

      
      this.readInt32();

      let parcel = this.readStringList();
      do_check_eq(parcel.length, 5);
      do_check_eq(parcel[0], GECKO_CARDLOCK_TO_FACILITIY_LOCK[aLock]);
      do_check_eq(parcel[1], aEnabled ? "1" : "0");
      do_check_eq(parcel[2], GECKO_CARDLOCK_TO_PASSWORD[aLock]);
      do_check_eq(parcel[3], serviceClass.toString());
      do_check_eq(parcel[4], aid);
    };

    let lock = {lockType: aLock,
                enabled: aEnabled};
    lock[GECKO_CARDLOCK_TO_PASSWORD_TYPE[aLock]] = aPassword;

    ril.iccSetCardLock(lock);
  }

  do_test(GECKO_CARDLOCK_PIN, pin, true);
  do_test(GECKO_CARDLOCK_PIN, pin, false);
  do_test(GECKO_CARDLOCK_FDN, pin2, true);
  do_test(GECKO_CARDLOCK_FDN, pin2, false);

  run_next_test();
});




add_test(function test_unlock_card_lock_corporateLocked() {
  let worker = newUint8Worker();
  let ril = worker.RIL;
  let buf = worker.Buf;
  const pin = "12345678";
  const puk = "12345678";

  let GECKO_CARDLOCK_TO_PASSWORD_TYPE = {};
  GECKO_CARDLOCK_TO_PASSWORD_TYPE[GECKO_CARDLOCK_NCK] = "pin";
  GECKO_CARDLOCK_TO_PASSWORD_TYPE[GECKO_CARDLOCK_CCK] = "pin";
  GECKO_CARDLOCK_TO_PASSWORD_TYPE[GECKO_CARDLOCK_SPCK] = "pin";
  GECKO_CARDLOCK_TO_PASSWORD_TYPE[GECKO_CARDLOCK_NCK_PUK] = "puk";
  GECKO_CARDLOCK_TO_PASSWORD_TYPE[GECKO_CARDLOCK_CCK_PUK] = "puk";
  GECKO_CARDLOCK_TO_PASSWORD_TYPE[GECKO_CARDLOCK_SPCK_PUK] = "puk";

  function do_test(aLock, aPassword) {
    buf.sendParcel = function fakeSendParcel () {
      
      do_check_eq(this.readInt32(), REQUEST_ENTER_NETWORK_DEPERSONALIZATION_CODE);

      
      this.readInt32();

      let lockType = GECKO_PERSO_LOCK_TO_CARD_PERSO_LOCK[aLock];
      
      do_check_eq(this.readInt32(), lockType);

      
      do_check_eq(this.readString(), aPassword);
    };

    let lock = {lockType: aLock};
    lock[GECKO_CARDLOCK_TO_PASSWORD_TYPE[aLock]] = aPassword;
    ril.iccUnlockCardLock(lock);
  }

  do_test(GECKO_CARDLOCK_NCK, pin);
  do_test(GECKO_CARDLOCK_CCK, pin);
  do_test(GECKO_CARDLOCK_SPCK, pin);
  do_test(GECKO_CARDLOCK_NCK_PUK, puk);
  do_test(GECKO_CARDLOCK_CCK_PUK, puk);
  do_test(GECKO_CARDLOCK_SPCK_PUK, puk);

  run_next_test();
});




add_test(function test_mcc_mnc_parsing() {
  let worker = newUint8Worker();
  let helper = worker.ICCUtilsHelper;

  function do_test(imsi, mncLength, expectedMcc, expectedMnc) {
    let result = helper.parseMccMncFromImsi(imsi, mncLength);

    if (!imsi) {
      do_check_eq(result, null);
      return;
    }

    do_check_eq(result.mcc, expectedMcc);
    do_check_eq(result.mnc, expectedMnc);
  }

  
  do_test(null, null, null, null);

  
  do_test("466923202422409", 0x02, "466", "92");
  do_test("466923202422409", 0x03, "466", "923");
  do_test("466923202422409", null, "466", "92");

  
  do_test("310260542718417", 0x02, "310", "26");
  do_test("310260542718417", 0x03, "310", "260");
  do_test("310260542718417", null, "310", "260");

  run_next_test();
 });

 


add_test(function test_reading_ad_and_parsing_mcc_mnc() {
  let worker = newUint8Worker();
  let record = worker.ICCRecordHelper;
  let helper = worker.GsmPDUHelper;
  let ril    = worker.RIL;
  let buf    = worker.Buf;
  let io     = worker.ICCIOHelper;

  function do_test(mncLengthInEf, imsi, expectedMcc, expectedMnc) {
    ril.iccInfoPrivate.imsi = imsi;

    io.loadTransparentEF = function fakeLoadTransparentEF(options) {
      let ad = [0x00, 0x00, 0x00];
      if (mncLengthInEf) {
        ad.push(mncLengthInEf);
      }

      
      buf.writeInt32(ad.length * 2);

      
      for (let i = 0; i < ad.length; i++) {
        helper.writeHexOctet(ad[i]);
      }

      
      buf.writeStringDelimiter(ad.length * 2);

      if (options.callback) {
        options.callback(options);
      }
    };

    record.readAD();

    do_check_eq(ril.iccInfo.mcc, expectedMcc);
    do_check_eq(ril.iccInfo.mnc, expectedMnc);
  }

  do_test(undefined, "466923202422409", "466", "92" );
  do_test(0x03,      "466923202422409", "466", "923");
  do_test(undefined, "310260542718417", "310", "260");
  do_test(0x02,      "310260542718417", "310", "26" );

  run_next_test();
});

add_test(function test_reading_optional_efs() {
  let worker = newUint8Worker();
  let record = worker.ICCRecordHelper;
  let gsmPdu = worker.GsmPDUHelper;
  let ril    = worker.RIL;
  let buf    = worker.Buf;
  let io     = worker.ICCIOHelper;

  function buildSST(supportedEf) {
    let sst = [];
    let len = supportedEf.length;
    for (let i = 0; i < len; i++) {
      let index, bitmask, iccService;
      if (ril.appType === CARD_APPTYPE_SIM) {
        iccService = GECKO_ICC_SERVICES.sim[supportedEf[i]];
        iccService -= 1;
        index = Math.floor(iccService / 4);
        bitmask = 2 << ((iccService % 4) << 1);
      } else if (ril.appType === CARD_APPTYPE_USIM){
        iccService = GECKO_ICC_SERVICES.usim[supportedEf[i]];
        iccService -= 1;
        index = Math.floor(iccService / 8);
        bitmask = 1 << ((iccService % 8) << 0);
      }

      if (sst) {
        sst[index] |= bitmask;
      }
    }
    return sst;
  }

  ril.updateCellBroadcastConfig = function fakeUpdateCellBroadcastConfig() {
    
  };

  function do_test(sst, supportedEf) {
    
    let testEf = supportedEf.slice(0);

    record.readMSISDN = function fakeReadMSISDN() {
      testEf.splice(testEf.indexOf("MSISDN"), 1);
    };

    record.readMBDN = function fakeReadMBDN() {
      testEf.splice(testEf.indexOf("MDN"), 1);
    };

    io.loadTransparentEF = function fakeLoadTransparentEF(options) {
      
      buf.writeInt32(sst.length * 2);

      
      for (let i = 0; i < sst.length; i++) {
         gsmPdu.writeHexOctet(sst[i] || 0);
      }

      
      buf.writeStringDelimiter(sst.length * 2);

      if (options.callback) {
        options.callback(options);
      }

      if (testEf.length !== 0) {
        do_print("Un-handled EF: " + JSON.stringify(testEf));
        do_check_true(false);
      }
    };

    record.readSST();
  }

  
  let supportedEf = ["MSISDN", "MDN"];
  ril.appType = CARD_APPTYPE_SIM;
  do_test(buildSST(supportedEf), supportedEf);
  ril.appType = CARD_APPTYPE_USIM;
  do_test(buildSST(supportedEf), supportedEf);

  run_next_test();
});
