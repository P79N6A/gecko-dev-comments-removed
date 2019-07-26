


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

function newUint8SupportOutgoingIndexWorker() {
  let worker = newWorker();
  let index = 4;          
  let buf = [0, 0, 0, 0]; 

  worker.Buf.writeUint8 = function (value) {
    if (worker.Buf.outgoingIndex >= buf.length) {
      buf.push(value);
    } else {
      buf[worker.Buf.outgoingIndex] = value;
    }

    worker.Buf.outgoingIndex++;
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




add_test(function test_send_stk_terminal_profile() {
  let worker = newUint8Worker();
  let ril = worker.RIL;
  let buf = worker.Buf;

  ril.sendStkTerminalProfile(STK_SUPPORTED_TERMINAL_PROFILE);

  buf.seekIncoming(8);
  let profile = buf.readString();
  for (let i = 0; i < STK_SUPPORTED_TERMINAL_PROFILE.length; i++) {
    do_check_eq(parseInt(profile.substring(2 * i, 2 * i + 2), 16),
                STK_SUPPORTED_TERMINAL_PROFILE[i]);
  }

  run_next_test();
});




add_test(function test_icc_get_card_lock_fdn() {
  let worker = newUint8Worker();
  let ril = worker.RIL;
  let buf = worker.Buf;

  buf.sendParcel = function () {
    
    do_check_eq(this.readUint32(), REQUEST_QUERY_FACILITY_LOCK)

    
    this.readUint32();

    
    do_check_eq(this.readUint32(), 4);

    
    do_check_eq(this.readString(), ICC_CB_FACILITY_FDN);

    
    do_check_eq(this.readString(), "");

    
    do_check_eq(this.readString(), (ICC_SERVICE_CLASS_VOICE |
                                    ICC_SERVICE_CLASS_DATA  |
                                    ICC_SERVICE_CLASS_FAX).toString());

    
    this.readUint32();

    run_next_test();
  };

  ril.iccGetCardLock({lockType: "fdn"});
});




add_test(function test_write_location_info_tlv() {
  let worker = newUint8Worker();
  let pduHelper = worker.GsmPDUHelper;
  let tlvHelper = worker.ComprehensionTlvHelper;

  
  let loc = {
    mcc: "466",
    mnc: "92",
    gsmLocationAreaCode : 10291,
    gsmCellId: 19072823
  };
  tlvHelper.writeLocationInfoTlv(loc);

  let tag = pduHelper.readHexOctet();
  do_check_eq(tag, COMPREHENSIONTLV_TAG_LOCATION_INFO |
                   COMPREHENSIONTLV_FLAG_CR);

  let length = pduHelper.readHexOctet();
  do_check_eq(length, 9);

  let mcc_mnc = pduHelper.readSwappedNibbleBcdString(3);
  do_check_eq(mcc_mnc, "46692");

  let lac = (pduHelper.readHexOctet() << 8) | pduHelper.readHexOctet();
  do_check_eq(lac, 10291);

  let cellId = (pduHelper.readHexOctet() << 24) |
               (pduHelper.readHexOctet() << 16) |
               (pduHelper.readHexOctet() << 8)  |
               (pduHelper.readHexOctet());
  do_check_eq(cellId, 19072823);

  
  loc = {
    mcc: "466",
    mnc: "02",
    gsmLocationAreaCode : 10291,
    gsmCellId: 65534
  };
  tlvHelper.writeLocationInfoTlv(loc);

  tag = pduHelper.readHexOctet();
  do_check_eq(tag, COMPREHENSIONTLV_TAG_LOCATION_INFO |
                   COMPREHENSIONTLV_FLAG_CR);

  length = pduHelper.readHexOctet();
  do_check_eq(length, 7);

  mcc_mnc = pduHelper.readSwappedNibbleBcdString(3);
  do_check_eq(mcc_mnc, "46602");

  lac = (pduHelper.readHexOctet() << 8) | pduHelper.readHexOctet();
  do_check_eq(lac, 10291);

  cellId = (pduHelper.readHexOctet() << 8) | (pduHelper.readHexOctet());
  do_check_eq(cellId, 65534);

  
  loc = {
    mcc: "466",
    mnc: "222",
    gsmLocationAreaCode : 10291,
    gsmCellId: 65534
  };
  tlvHelper.writeLocationInfoTlv(loc);

  tag = pduHelper.readHexOctet();
  do_check_eq(tag, COMPREHENSIONTLV_TAG_LOCATION_INFO |
                   COMPREHENSIONTLV_FLAG_CR);

  length = pduHelper.readHexOctet();
  do_check_eq(length, 7);

  mcc_mnc = pduHelper.readSwappedNibbleBcdString(3);
  do_check_eq(mcc_mnc, "466222");

  lac = (pduHelper.readHexOctet() << 8) | pduHelper.readHexOctet();
  do_check_eq(lac, 10291);

  cellId = (pduHelper.readHexOctet() << 8) | (pduHelper.readHexOctet());
  do_check_eq(cellId, 65534);

  run_next_test();
});




add_test(function test_write_disconnecting_cause() {
  let worker = newUint8Worker();
  let pduHelper = worker.GsmPDUHelper;
  let tlvHelper = worker.ComprehensionTlvHelper;

  tlvHelper.writeCauseTlv(RIL_ERROR_TO_GECKO_ERROR[ERROR_GENERIC_FAILURE]);
  let tag = pduHelper.readHexOctet();
  do_check_eq(tag, COMPREHENSIONTLV_TAG_CAUSE | COMPREHENSIONTLV_FLAG_CR);
  let len = pduHelper.readHexOctet();
  do_check_eq(len, 2);  
  let standard = pduHelper.readHexOctet();
  do_check_eq(standard, 0x60);
  let cause = pduHelper.readHexOctet();
  do_check_eq(cause, 0x80 | ERROR_GENERIC_FAILURE);

  run_next_test();
});




add_test(function test_get_size_of_length_octets() {
  let worker = newUint8Worker();
  let tlvHelper = worker.ComprehensionTlvHelper;

  let length = 0x70;
  do_check_eq(tlvHelper.getSizeOfLengthOctets(length), 1);

  length = 0x80;
  do_check_eq(tlvHelper.getSizeOfLengthOctets(length), 2);

  length = 0x180;
  do_check_eq(tlvHelper.getSizeOfLengthOctets(length), 3);

  length = 0x18000;
  do_check_eq(tlvHelper.getSizeOfLengthOctets(length), 4);

  run_next_test();
});




add_test(function test_write_length() {
  let worker = newUint8Worker();
  let pduHelper = worker.GsmPDUHelper;
  let tlvHelper = worker.ComprehensionTlvHelper;

  let length = 0x70;
  tlvHelper.writeLength(length);
  do_check_eq(pduHelper.readHexOctet(), length);

  length = 0x80;
  tlvHelper.writeLength(length);
  do_check_eq(pduHelper.readHexOctet(), 0x81);
  do_check_eq(pduHelper.readHexOctet(), length);

  length = 0x180;
  tlvHelper.writeLength(length);
  do_check_eq(pduHelper.readHexOctet(), 0x82);
  do_check_eq(pduHelper.readHexOctet(), (length >> 8) & 0xff);
  do_check_eq(pduHelper.readHexOctet(), length & 0xff);

  length = 0x18000;
  tlvHelper.writeLength(length);
  do_check_eq(pduHelper.readHexOctet(), 0x83);
  do_check_eq(pduHelper.readHexOctet(), (length >> 16) & 0xff);
  do_check_eq(pduHelper.readHexOctet(), (length >> 8) & 0xff);
  do_check_eq(pduHelper.readHexOctet(), length & 0xff);

  run_next_test();
});




add_test(function test_stk_proactive_command_refresh() {
  let worker = newUint8Worker();
  let pduHelper = worker.GsmPDUHelper;
  let berHelper = worker.BerTlvHelper;
  let stkHelper = worker.StkProactiveCmdHelper;

  let refresh_1 = [
    0xD0,
    0x10,
    0x81, 0x03, 0x01, 0x01, 0x01,
    0x82, 0x02, 0x81, 0x82,
    0x92, 0x05, 0x01, 0x3F, 0x00, 0x2F, 0xE2];

  for (let i = 0; i < refresh_1.length; i++) {
    pduHelper.writeHexOctet(refresh_1[i]);
  }

  let berTlv = berHelper.decode(refresh_1.length);
  let ctlvs = berTlv.value;
  let tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_COMMAND_DETAILS, ctlvs);
  do_check_eq(tlv.value.commandNumber, 0x01);
  do_check_eq(tlv.value.typeOfCommand, 0x01);
  do_check_eq(tlv.value.commandQualifier, 0x01);

  tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_FILE_LIST, ctlvs);
  do_check_eq(tlv.value.fileList, "3F002FE2");

  run_next_test();
});




add_test(function test_stk_proactive_command_play_tone() {
  let worker = newUint8Worker();
  let pduHelper = worker.GsmPDUHelper;
  let berHelper = worker.BerTlvHelper;
  let stkHelper = worker.StkProactiveCmdHelper;

  let tone_1 = [
    0xD0,
    0x1B,
    0x81, 0x03, 0x01, 0x20, 0x00,
    0x82, 0x02, 0x81, 0x03,
    0x85, 0x09, 0x44, 0x69, 0x61, 0x6C, 0x20, 0x54, 0x6F, 0x6E, 0x65,
    0x8E, 0x01, 0x01,
    0x84, 0x02, 0x01, 0x05];

  for (let i = 0; i < tone_1.length; i++) {
    pduHelper.writeHexOctet(tone_1[i]);
  }

  let berTlv = berHelper.decode(tone_1.length);
  let ctlvs = berTlv.value;
  let tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_COMMAND_DETAILS, ctlvs);
  do_check_eq(tlv.value.commandNumber, 0x01);
  do_check_eq(tlv.value.typeOfCommand, 0x20);
  do_check_eq(tlv.value.commandQualifier, 0x00);

  tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_ALPHA_ID, ctlvs);
  do_check_eq(tlv.value.identifier, "Dial Tone");

  tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_TONE, ctlvs);
  do_check_eq(tlv.value.tone, STK_TONE_TYPE_DIAL_TONE);

  tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_DURATION, ctlvs);
  do_check_eq(tlv.value.timeUnit, STK_TIME_UNIT_SECOND);
  do_check_eq(tlv.value.timeInterval, 5);

  run_next_test();
});




add_test(function test_stk_proactive_command_poll_interval() {
  let worker = newUint8Worker();
  let pduHelper = worker.GsmPDUHelper;
  let berHelper = worker.BerTlvHelper;
  let stkHelper = worker.StkProactiveCmdHelper;

  let poll_1 = [
    0xD0,
    0x0D,
    0x81, 0x03, 0x01, 0x03, 0x00,
    0x82, 0x02, 0x81, 0x82,
    0x84, 0x02, 0x01, 0x14];

  for (let i = 0; i < poll_1.length; i++) {
    pduHelper.writeHexOctet(poll_1[i]);
  }

  let berTlv = berHelper.decode(poll_1.length);
  let ctlvs = berTlv.value;
  let tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_COMMAND_DETAILS, ctlvs);
  do_check_eq(tlv.value.commandNumber, 0x01);
  do_check_eq(tlv.value.typeOfCommand, 0x03);
  do_check_eq(tlv.value.commandQualifier, 0x00);

  tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_DURATION, ctlvs);
  do_check_eq(tlv.value.timeUnit, STK_TIME_UNIT_SECOND);
  do_check_eq(tlv.value.timeInterval, 0x14);

  run_next_test();
});




add_test(function test_read_septets_to_string() {
  let worker = newUint8Worker();
  let pduHelper = worker.GsmPDUHelper;
  let berHelper = worker.BerTlvHelper;
  let stkHelper = worker.StkProactiveCmdHelper;

  let display_text_1 = [
    0xd0,
    0x28,
    0x81, 0x03, 0x01, 0x21, 0x80,
    0x82, 0x02, 0x81, 0x02,
    0x0d, 0x1d, 0x00, 0xd3, 0x30, 0x9b, 0xfc, 0x06, 0xc9, 0x5c, 0x30, 0x1a,
    0xa8, 0xe8, 0x02, 0x59, 0xc3, 0xec, 0x34, 0xb9, 0xac, 0x07, 0xc9, 0x60,
    0x2f, 0x58, 0xed, 0x15, 0x9b, 0xb9, 0x40,
  ];

  for (let i = 0; i < display_text_1.length; i++) {
    pduHelper.writeHexOctet(display_text_1[i]);
  }

  let berTlv = berHelper.decode(display_text_1.length);
  let ctlvs = berTlv.value;
  let tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_TEXT_STRING, ctlvs);
  do_check_eq(tlv.value.textString, "Saldo 2.04 E. Validez 20/05/13. ");

  run_next_test();
});

add_test(function test_stk_proactive_command_event_list() {
  let worker = newUint8Worker();
  let pduHelper = worker.GsmPDUHelper;
  let berHelper = worker.BerTlvHelper;
  let stkHelper = worker.StkProactiveCmdHelper;

  let event_1 = [
    0xD0,
    0x0F,
    0x81, 0x03, 0x01, 0x05, 0x00,
    0x82, 0x02, 0x81, 0x82,
    0x99, 0x04, 0x00, 0x01, 0x02, 0x03];

  for (let i = 0; i < event_1.length; i++) {
    pduHelper.writeHexOctet(event_1[i]);
  }

  let berTlv = berHelper.decode(event_1.length);
  let ctlvs = berTlv.value;
  let tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_COMMAND_DETAILS, ctlvs);
  do_check_eq(tlv.value.commandNumber, 0x01);
  do_check_eq(tlv.value.typeOfCommand, 0x05);
  do_check_eq(tlv.value.commandQualifier, 0x00);

  tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_EVENT_LIST, ctlvs);
  do_check_eq(Array.isArray(tlv.value.eventList), true);
  for (let i = 0; i < tlv.value.eventList.length; i++) {
    do_check_eq(tlv.value.eventList[i], i);
  }

  run_next_test();
});




add_test(function test_stk_proactive_command_get_input() {
  let worker = newUint8Worker();
  let pduHelper = worker.GsmPDUHelper;
  let berHelper = worker.BerTlvHelper;
  let stkHelper = worker.StkProactiveCmdHelper;
  let stkCmdHelper = worker.StkCommandParamsFactory;

  let get_input_1 = [
    0xD0,
    0x1E,
    0x81, 0x03, 0x01, 0x23, 0x8F,
    0x82, 0x02, 0x81, 0x82,
    0x8D, 0x05, 0x04, 0x54, 0x65, 0x78, 0x74,
    0x91, 0x02, 0x01, 0x10,
    0x17, 0x08, 0x04, 0x44, 0x65, 0x66, 0x61, 0x75, 0x6C, 0x74];

  for (let i = 0; i < get_input_1.length; i++) {
    pduHelper.writeHexOctet(get_input_1[i]);
  }

  let berTlv = berHelper.decode(get_input_1.length);
  let ctlvs = berTlv.value;
  let tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_COMMAND_DETAILS, ctlvs);
  do_check_eq(tlv.value.commandNumber, 0x01);
  do_check_eq(tlv.value.typeOfCommand, STK_CMD_GET_INPUT);

  let input = stkCmdHelper.createParam(tlv.value, ctlvs);
  do_check_eq(input.text, "Text");
  do_check_eq(input.isAlphabet, true);
  do_check_eq(input.isUCS2, true);
  do_check_eq(input.hideInput, true);
  do_check_eq(input.isPacked, true);
  do_check_eq(input.isHelpAvailable, true);
  do_check_eq(input.minLength, 0x01);
  do_check_eq(input.maxLength, 0x10);
  do_check_eq(input.defaultText, "Default");

  let get_input_2 = [
    0xD0,
    0x11,
    0x81, 0x03, 0x01, 0x23, 0x00,
    0x82, 0x02, 0x81, 0x82,
    0x8D, 0x00,
    0x91, 0x02, 0x01, 0x10,
    0x17, 0x00];

  for (let i = 0; i < get_input_2.length; i++) {
    pduHelper.writeHexOctet(get_input_2[i]);
  }

  berTlv = berHelper.decode(get_input_2.length);
  ctlvs = berTlv.value;
  tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_COMMAND_DETAILS, ctlvs);
  do_check_eq(tlv.value.commandNumber, 0x01);
  do_check_eq(tlv.value.typeOfCommand, STK_CMD_GET_INPUT);

  input = stkCmdHelper.createParam(tlv.value, ctlvs);
  do_check_eq(input.text, null);
  do_check_eq(input.minLength, 0x01);
  do_check_eq(input.maxLength, 0x10);
  do_check_eq(input.defaultText, null);

  run_next_test();
});

add_test(function test_spn_display_condition() {
  let worker = newWorker({
    postRILMessage: function fakePostRILMessage(data) {
      
    },
    postMessage: function fakePostMessage(message) {
      
    }
  });
  let RIL = worker.RIL;
  let ICCUtilsHelper = worker.ICCUtilsHelper;

  
  do_check_eq(ICCUtilsHelper.updateDisplayCondition(), true);
  do_check_eq(RIL.iccInfo.isDisplayNetworkNameRequired, true);
  do_check_eq(RIL.iccInfo.isDisplaySpnRequired, false);

  
  function testDisplayCondition(iccDisplayCondition,
                                iccMcc, iccMnc, plmnMcc, plmnMnc,
                                expectedIsDisplayNetworkNameRequired,
                                expectedIsDisplaySPNRequired,
                                callback) {
    RIL.iccInfoPrivate.SPN = {
      spnDisplayCondition: iccDisplayCondition
    };
    RIL.iccInfo = {
      mcc: iccMcc,
      mnc: iccMnc
    };
    RIL.operator = {
      mcc: plmnMcc,
      mnc: plmnMnc
    };

    do_check_eq(ICCUtilsHelper.updateDisplayCondition(), true);
    do_check_eq(RIL.iccInfo.isDisplayNetworkNameRequired, expectedIsDisplayNetworkNameRequired);
    do_check_eq(RIL.iccInfo.isDisplaySpnRequired, expectedIsDisplaySPNRequired);
    do_timeout(0, callback);
  };

  function testDisplayConditions(func, caseArray, oncomplete) {
    (function do_call(index) {
      let next = index < (caseArray.length - 1) ? do_call.bind(null, index + 1) : oncomplete;
      caseArray[index].push(next);
      func.apply(null, caseArray[index]);
    })(0);
  }

  testDisplayConditions(testDisplayCondition, [
    [1, 123, 456, 123, 456, true, true],
    [0, 123, 456, 123, 456, false, true],
    [2, 123, 456, 123, 457, false, false],
    [0, 123, 456, 123, 457, false, true],
  ], run_next_test);
});




add_test(function test_stk_proactive_command_more_time() {
  let worker = newUint8Worker();
  let pduHelper = worker.GsmPDUHelper;
  let berHelper = worker.BerTlvHelper;
  let stkHelper = worker.StkProactiveCmdHelper;

  let more_time_1 = [
    0xD0,
    0x09,
    0x81, 0x03, 0x01, 0x02, 0x00,
    0x82, 0x02, 0x81, 0x82];

  for(let i = 0 ; i < more_time_1.length; i++) {
    pduHelper.writeHexOctet(more_time_1[i]);
  }

  let berTlv = berHelper.decode(more_time_1.length);
  let ctlvs = berTlv.value;
  let tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_COMMAND_DETAILS, ctlvs);
  do_check_eq(tlv.value.commandNumber, 0x01);
  do_check_eq(tlv.value.typeOfCommand, STK_CMD_MORE_TIME);
  do_check_eq(tlv.value.commandQualifier, 0x00);

  run_next_test();
});




add_test(function test_stk_proactive_command_set_up_call() {
  let worker = newUint8Worker();
  let pduHelper = worker.GsmPDUHelper;
  let berHelper = worker.BerTlvHelper;
  let stkHelper = worker.StkProactiveCmdHelper;
  let cmdFactory = worker.StkCommandParamsFactory;

  let set_up_call_1 = [
    0xD0,
    0x29,
    0x81, 0x03, 0x01, 0x10, 0x04,
    0x82, 0x02, 0x81, 0x82,
    0x05, 0x0A, 0x44, 0x69, 0x73, 0x63, 0x6F, 0x6E, 0x6E, 0x65, 0x63, 0x74,
    0x86, 0x09, 0x81, 0x10, 0x32, 0x04, 0x21, 0x43, 0x65, 0x1C, 0x2C,
    0x05, 0x07, 0x4D, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65];

  for (let i = 0 ; i < set_up_call_1.length; i++) {
    pduHelper.writeHexOctet(set_up_call_1[i]);
  }

  let berTlv = berHelper.decode(set_up_call_1.length);
  let ctlvs = berTlv.value;
  let tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_COMMAND_DETAILS, ctlvs);
  do_check_eq(tlv.value.commandNumber, 0x01);
  do_check_eq(tlv.value.typeOfCommand, STK_CMD_SET_UP_CALL);

  let setupCall = cmdFactory.createParam(tlv.value, ctlvs);
  do_check_eq(setupCall.address, "012340123456,1,2");
  do_check_eq(setupCall.confirmMessage, "Disconnect");
  do_check_eq(setupCall.callMessage, "Message");

  run_next_test();
});

add_test(function test_read_pnn() {
  let worker = newUint8Worker();
  let helper = worker.GsmPDUHelper;
  let record = worker.ICCRecordHelper;
  let buf    = worker.Buf;
  let io     = worker.ICCIOHelper;
  let ril    = worker.RIL;

  io.loadLinearFixedEF = function fakeLoadLinearFixedEF(options) {
    let records = [
      
      [0x43, 0x06, 0x85, 0xCC, 0xB7, 0xFB, 0x1C, 0x03,
       0x45, 0x07, 0x86, 0x53, 0xF4, 0x5B, 0x4E, 0x8F, 0x01,
       0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF],
      
      [0x43, 0x06, 0x85, 0xCC, 0xB7, 0xFB, 0x2C, 0x03,
       0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
       0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF],
      
      [0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
       0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
       0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF],
    ];

    
    options.totalRecords = records.length;
    options.recordSize = records[0].length;

    options.p1 = options.p1 || 1;

    let record = records[options.p1 - 1];

    
    buf.writeUint32(record.length * 2);

    
    for (let i = 0; i < record.length; i++) {
      helper.writeHexOctet(record[i]);
    }

    
    buf.writeStringDelimiter(record.length * 2);

    if (options.callback) {
      options.callback(options);
    }
  };

  io.loadNextRecord = function fakeLoadNextRecord(options) {
    options.p1++;
    io.loadLinearFixedEF(options);
  };

  record.readPNN();

  do_check_eq(ril.iccInfoPrivate.PNN.length, 2);
  do_check_eq(ril.iccInfoPrivate.PNN[0].fullName, "Long1");
  do_check_eq(ril.iccInfoPrivate.PNN[0].shortName, "Short1");
  do_check_eq(ril.iccInfoPrivate.PNN[1].fullName, "Long2");
  do_check_eq(ril.iccInfoPrivate.PNN[1].shortName, undefined);

  run_next_test();
});

add_test(function read_network_name() {
  let worker = newUint8Worker();
  let helper = worker.GsmPDUHelper;
  let buf = worker.Buf;

  
  function writeNetworkName(isUCS2, requireCi, name) {
    let codingOctet = 0x80;
    let len;
    if (requireCi) {
      codingOctet |= 0x08;
    }

    if (isUCS2) {
      codingOctet |= 0x10;
      len = name.length * 2;
    } else {
      let spare = (8 - (name.length * 7) % 8) % 8;
      codingOctet |= spare;
      len = Math.ceil(name.length * 7 / 8);
    }
    helper.writeHexOctet(codingOctet);

    if (isUCS2) {
      helper.writeUCS2String(name);
    } else {
      helper.writeStringAsSeptets(name, 0, 0, 0);
    }

    return len + 1; 
  }

  function testNetworkName(isUCS2, requireCi, name) {
    let len = writeNetworkName(isUCS2, requireCi, name);
    do_check_eq(helper.readNetworkName(len), name);
  }

  testNetworkName( true,  true, "Test Network Name1");
  testNetworkName( true, false, "Test Network Name2");
  testNetworkName(false,  true, "Test Network Name3");
  testNetworkName(false, false, "Test Network Name4");

  run_next_test();
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




add_test(function test_stk_proactive_command_timer_management() {
  let worker = newUint8Worker();
  let pduHelper = worker.GsmPDUHelper;
  let berHelper = worker.BerTlvHelper;
  let stkHelper = worker.StkProactiveCmdHelper;

  
  let timer_management_1 = [
    0xD0,
    0x11,
    0x81, 0x03, 0x01, 0x27, 0x00,
    0x82, 0x02, 0x81, 0x82,
    0xA4, 0x01, 0x01,
    0xA5, 0x03, 0x10, 0x20, 0x30
  ];

  for(let i = 0 ; i < timer_management_1.length; i++) {
    pduHelper.writeHexOctet(timer_management_1[i]);
  }

  let berTlv = berHelper.decode(timer_management_1.length);
  let ctlvs = berTlv.value;
  let tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_COMMAND_DETAILS, ctlvs);
  do_check_eq(tlv.value.commandNumber, 0x01);
  do_check_eq(tlv.value.typeOfCommand, STK_CMD_TIMER_MANAGEMENT);
  do_check_eq(tlv.value.commandQualifier, STK_TIMER_START);

  tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_TIMER_IDENTIFIER, ctlvs);
  do_check_eq(tlv.value.timerId, 0x01);

  tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_TIMER_VALUE, ctlvs);
  do_check_eq(tlv.value.timerValue, (0x01 * 60 * 60) + (0x02 * 60) + 0x03);

  
  let timer_management_2 = [
    0xD0,
    0x0C,
    0x81, 0x03, 0x01, 0x27, 0x01,
    0x82, 0x02, 0x81, 0x82,
    0xA4, 0x01, 0x01
  ];

  for(let i = 0 ; i < timer_management_2.length; i++) {
    pduHelper.writeHexOctet(timer_management_2[i]);
  }

  berTlv = berHelper.decode(timer_management_2.length);
  ctlvs = berTlv.value;
  tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_COMMAND_DETAILS, ctlvs);
  do_check_eq(tlv.value.commandNumber, 0x01);
  do_check_eq(tlv.value.typeOfCommand, STK_CMD_TIMER_MANAGEMENT);
  do_check_eq(tlv.value.commandQualifier, STK_TIMER_DEACTIVATE);

  tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_TIMER_IDENTIFIER, ctlvs);
  do_check_eq(tlv.value.timerId, 0x01);

  run_next_test();
});




add_test(function test_stk_proactive_command_provide_local_information() {
  let worker = newUint8Worker();
  let pduHelper = worker.GsmPDUHelper;
  let berHelper = worker.BerTlvHelper;
  let stkHelper = worker.StkProactiveCmdHelper;

  
  let local_info_1 = [
    0xD0,
    0x09,
    0x81, 0x03, 0x01, 0x26, 0x01,
    0x82, 0x02, 0x81, 0x82];

  for (let i = 0; i < local_info_1.length; i++) {
    pduHelper.writeHexOctet(local_info_1[i]);
  }

  let berTlv = berHelper.decode(local_info_1.length);
  let ctlvs = berTlv.value;
  let tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_COMMAND_DETAILS, ctlvs);
  do_check_eq(tlv.value.commandNumber, 0x01);
  do_check_eq(tlv.value.typeOfCommand, STK_CMD_PROVIDE_LOCAL_INFO);
  do_check_eq(tlv.value.commandQualifier, STK_LOCAL_INFO_IMEI);

  
  let local_info_2 = [
    0xD0,
    0x09,
    0x81, 0x03, 0x01, 0x26, 0x03,
    0x82, 0x02, 0x81, 0x82];

  for (let i = 0; i < local_info_2.length; i++) {
    pduHelper.writeHexOctet(local_info_2[i]);
  }

  berTlv = berHelper.decode(local_info_2.length);
  ctlvs = berTlv.value;
  tlv = stkHelper.searchForTag(COMPREHENSIONTLV_TAG_COMMAND_DETAILS, ctlvs);
  do_check_eq(tlv.value.commandNumber, 0x01);
  do_check_eq(tlv.value.typeOfCommand, STK_CMD_PROVIDE_LOCAL_INFO);
  do_check_eq(tlv.value.commandQualifier, STK_LOCAL_INFO_DATE_TIME_ZONE);

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




add_test(function test_stk_event_download_location_status() {
  let worker = newUint8SupportOutgoingIndexWorker();
  let buf = worker.Buf;
  let pduHelper = worker.GsmPDUHelper;

  buf.sendParcel = function () {
    
    do_check_eq(this.readUint32(), REQUEST_STK_SEND_ENVELOPE_COMMAND);

    
    this.readUint32();

    
    
    
    
    do_check_eq(this.readUint32(), 42);

    
    do_check_eq(pduHelper.readHexOctet(), BER_EVENT_DOWNLOAD_TAG);

    
    
    
    
    do_check_eq(pduHelper.readHexOctet(), 19);

    
    do_check_eq(pduHelper.readHexOctet(), COMPREHENSIONTLV_TAG_DEVICE_ID |
                                          COMPREHENSIONTLV_FLAG_CR);
    do_check_eq(pduHelper.readHexOctet(), 2);
    do_check_eq(pduHelper.readHexOctet(), STK_DEVICE_ID_ME);
    do_check_eq(pduHelper.readHexOctet(), STK_DEVICE_ID_SIM);

    
    do_check_eq(pduHelper.readHexOctet(), COMPREHENSIONTLV_TAG_EVENT_LIST |
                                          COMPREHENSIONTLV_FLAG_CR);
    do_check_eq(pduHelper.readHexOctet(), 1);
    do_check_eq(pduHelper.readHexOctet(), STK_EVENT_TYPE_LOCATION_STATUS);

    
    do_check_eq(pduHelper.readHexOctet(), COMPREHENSIONTLV_TAG_LOCATION_STATUS |
                                          COMPREHENSIONTLV_FLAG_CR);
    do_check_eq(pduHelper.readHexOctet(), 1);
    do_check_eq(pduHelper.readHexOctet(), STK_SERVICE_STATE_NORMAL);

    
    do_check_eq(pduHelper.readHexOctet(), COMPREHENSIONTLV_TAG_LOCATION_INFO |
                                          COMPREHENSIONTLV_FLAG_CR);
    do_check_eq(pduHelper.readHexOctet(), 7);

    do_check_eq(pduHelper.readHexOctet(), 0x21); 
    do_check_eq(pduHelper.readHexOctet(), 0x63);
    do_check_eq(pduHelper.readHexOctet(), 0x54);
    do_check_eq(pduHelper.readHexOctet(), 0); 
    do_check_eq(pduHelper.readHexOctet(), 0);
    do_check_eq(pduHelper.readHexOctet(), 0); 
    do_check_eq(pduHelper.readHexOctet(), 0);

    run_next_test();
  };

  let event = {
    eventType: STK_EVENT_TYPE_LOCATION_STATUS,
    locationStatus: STK_SERVICE_STATE_NORMAL,
    locationInfo: {
      mcc: "123",
      mnc: "456",
      gsmLocationAreaCode: 0,
      gsmCellId: 0
    }
  };
  worker.RIL.sendStkEventDownload({
    event: event
  });
});




add_test(function test_stk_terminal_response() {
  let worker = newUint8SupportOutgoingIndexWorker();
  let buf = worker.Buf;
  let pduHelper = worker.GsmPDUHelper;

  buf.sendParcel = function () {
    
    do_check_eq(this.readUint32(), REQUEST_STK_SEND_TERMINAL_RESPONSE);

    
    this.readUint32();

    
    
    
    
    do_check_eq(this.readUint32(), 44);

    
    do_check_eq(pduHelper.readHexOctet(), COMPREHENSIONTLV_TAG_COMMAND_DETAILS |
                                          COMPREHENSIONTLV_FLAG_CR);
    do_check_eq(pduHelper.readHexOctet(), 3);
    do_check_eq(pduHelper.readHexOctet(), 0x01);
    do_check_eq(pduHelper.readHexOctet(), STK_CMD_PROVIDE_LOCAL_INFO);
    do_check_eq(pduHelper.readHexOctet(), STK_LOCAL_INFO_NNA);

    
    do_check_eq(pduHelper.readHexOctet(), COMPREHENSIONTLV_TAG_DEVICE_ID);
    do_check_eq(pduHelper.readHexOctet(), 2);
    do_check_eq(pduHelper.readHexOctet(), STK_DEVICE_ID_ME);
    do_check_eq(pduHelper.readHexOctet(), STK_DEVICE_ID_SIM);

    
    do_check_eq(pduHelper.readHexOctet(), COMPREHENSIONTLV_TAG_RESULT |
                                          COMPREHENSIONTLV_FLAG_CR);
    do_check_eq(pduHelper.readHexOctet(), 1);
    do_check_eq(pduHelper.readHexOctet(), STK_RESULT_OK);

    
    do_check_eq(pduHelper.readHexOctet(), COMPREHENSIONTLV_TAG_TEXT_STRING |
                                          COMPREHENSIONTLV_FLAG_CR);
    do_check_eq(pduHelper.readHexOctet(), 8);
    do_check_eq(pduHelper.readHexOctet(), STK_TEXT_CODING_GSM_7BIT_PACKED);
    do_check_eq(pduHelper.readSeptetsToString(7, 0, PDU_NL_IDENTIFIER_DEFAULT,
                PDU_NL_IDENTIFIER_DEFAULT), "Mozilla");

    run_next_test();
  };

  let response = {
    command: {
      commandNumber: 0x01,
      typeOfCommand: STK_CMD_PROVIDE_LOCAL_INFO,
      commandQualifier: STK_LOCAL_INFO_NNA,
      options: {
        isPacked: true
      }
    },
    input: "Mozilla",
    resultCode: STK_RESULT_OK
  };
  worker.RIL.sendStkTerminalResponse(response);
});




add_test(function test_stk_event_download_language_selection() {
  let worker = newUint8SupportOutgoingIndexWorker();
  let buf = worker.Buf;
  let pduHelper = worker.GsmPDUHelper;

  buf.sendParcel = function () {
    
    do_check_eq(this.readUint32(), REQUEST_STK_SEND_ENVELOPE_COMMAND);

    
    this.readUint32();

    
    
    
    do_check_eq(this.readUint32(), 26);

    
    do_check_eq(pduHelper.readHexOctet(), BER_EVENT_DOWNLOAD_TAG);

    
    
    
    do_check_eq(pduHelper.readHexOctet(), 11);

    
    do_check_eq(pduHelper.readHexOctet(), COMPREHENSIONTLV_TAG_DEVICE_ID |
                                          COMPREHENSIONTLV_FLAG_CR);
    do_check_eq(pduHelper.readHexOctet(), 2);
    do_check_eq(pduHelper.readHexOctet(), STK_DEVICE_ID_ME);
    do_check_eq(pduHelper.readHexOctet(), STK_DEVICE_ID_SIM);

    
    do_check_eq(pduHelper.readHexOctet(), COMPREHENSIONTLV_TAG_EVENT_LIST |
                                          COMPREHENSIONTLV_FLAG_CR);
    do_check_eq(pduHelper.readHexOctet(), 1);
    do_check_eq(pduHelper.readHexOctet(), STK_EVENT_TYPE_LANGUAGE_SELECTION);

    
    do_check_eq(pduHelper.readHexOctet(), COMPREHENSIONTLV_TAG_LANGUAGE);
    do_check_eq(pduHelper.readHexOctet(), 2);
    do_check_eq(pduHelper.read8BitUnpackedToString(2), "zh");

    run_next_test();
  };

  let event = {
    eventType: STK_EVENT_TYPE_LANGUAGE_SELECTION,
    language: "zh"
  };
  worker.RIL.sendStkEventDownload({
    event: event
  });
});




add_test(function test_stk_event_download_idle_screen_available() {
  let worker = newUint8SupportOutgoingIndexWorker();
  let buf = worker.Buf;
  let pduHelper = worker.GsmPDUHelper;

  buf.sendParcel = function () {
    
    do_check_eq(this.readUint32(), REQUEST_STK_SEND_ENVELOPE_COMMAND);

    
    this.readUint32();

    
    do_check_eq(this.readUint32(), 18);

    
    do_check_eq(pduHelper.readHexOctet(), BER_EVENT_DOWNLOAD_TAG);

    
    do_check_eq(pduHelper.readHexOctet(), 7);

    
    do_check_eq(pduHelper.readHexOctet(), COMPREHENSIONTLV_TAG_DEVICE_ID |
                                          COMPREHENSIONTLV_FLAG_CR);
    do_check_eq(pduHelper.readHexOctet(), 2);
    do_check_eq(pduHelper.readHexOctet(), STK_DEVICE_ID_DISPLAY);
    do_check_eq(pduHelper.readHexOctet(), STK_DEVICE_ID_SIM);

    
    do_check_eq(pduHelper.readHexOctet(), COMPREHENSIONTLV_TAG_EVENT_LIST |
                                          COMPREHENSIONTLV_FLAG_CR);
    do_check_eq(pduHelper.readHexOctet(), 1);
    do_check_eq(pduHelper.readHexOctet(), STK_EVENT_TYPE_IDLE_SCREEN_AVAILABLE);

    run_next_test();
  };

  let event = {
    eventType: STK_EVENT_TYPE_IDLE_SCREEN_AVAILABLE
  };
  worker.RIL.sendStkEventDownload({
    event: event
  });
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

    
    buf.writeUint32(email_1.length * 2);

    
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




add_test(function test_read_email() {
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

    
    buf.writeUint32(anr_1.length * 2);

    
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
    
    do_check_eq(this.readUint32(), REQUEST_SIM_IO);

    
    this.readUint32();

    
    do_check_eq(this.readUint32(), ICC_COMMAND_UPDATE_RECORD);

    
    do_check_eq(this.readUint32(), fileId);

    
    do_check_eq(this.readString(), EF_PATH_MF_SIM + EF_PATH_DF_TELECOM);

    
    do_check_eq(this.readUint32(), 1);

    
    do_check_eq(this.readUint32(), READ_RECORD_ABSOLUTE_MODE);

    
    do_check_eq(this.readUint32(), 0x20);

    
    let contact = pdu.readAlphaIdDiallingNumber(0x20);
    do_check_eq(contact.alphaId, "test");
    do_check_eq(contact.number, "123456");

    
    if (fileId == ICC_EF_ADN) {
      do_check_eq(this.readString(), "");
    } else {
      do_check_eq(this.readString(), "1111");
    }

    
    this.readUint32();

    if (fileId == ICC_EF_FDN) {
      run_next_test();
    }
  };

  fileId = ICC_EF_ADN;
  record.updateADNLike(fileId,
                       {recordId: 1, alphaId: "test", number: "123456"},
                       "");

  fileId = ICC_EF_FDN;
  record.updateADNLike(fileId,
                       {recordId: 1, alphaId: "test", number: "123456"},
                       "1111");
});




add_test(function test_get_free_record_id() {
  let worker = newUint8Worker();
  let helper = worker.GsmPDUHelper;
  let record = worker.ICCRecordHelper;
  let buf    = worker.Buf;
  let io     = worker.ICCIOHelper;

  function writeRecord (record) {
    
    buf.writeUint32(record.length * 2);

    for (let i = 0; i < record.length; i++) {
      helper.writeHexOctet(record[i]);
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
  record.getFreeRecordId(
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

  
  contactHelper.getContactFieldRecordId = function (pbr, contact, field, onsuccess, onerror) {
    onsuccess(1);
  };

  record.readPBR = function readPBR(onsuccess, onerror) {
    onsuccess({adn:{}, email: {}, anr0: {}});
  };

  record.readADNLike = function readADNLike(fileId, onsuccess, onerror) {
    onsuccess([{alphaId: "name", number: "111111"}])
  };

  record.readEmail = function readEmail(fileId, fileType, recordNumber, onsuccess, onerror) {
    onsuccess("hello@mail.com");
  };

  record.readANR = function readANR(fileId, fileType, recordNumber, onsuccess, onerror) {
    onsuccess("123456");
  };

  let successCb = function successCb(contacts) {
    let contact = contacts[0];
    do_check_eq(contact.alphaId, "name");
    do_check_eq(contact.number, "111111");
    do_check_eq(contact.email, "hello@mail.com");
    do_check_eq(contact.anr[0], "123456");
    run_next_test();
  };

  let errorCb = function errorCb(errorMsg) {
    do_print(errorMsg);
    do_check_true(false);
    run_next_test();
  };

  contactHelper.readICCContacts(CARD_APPTYPE_USIM, "ADN", successCb, errorCb);
});




add_test(function test_update_icc_contact() {
  let worker = newUint8Worker();
  let record = worker.ICCRecordHelper;
  let contactHelper = worker.ICCContactHelper;
  let contactType;

  function do_test(aContact, aContactType, aFileId, aPin2) {
    record.updateADNLike = function (fileId, contact, pin2, onsuccess, onerror) {
      do_check_eq(fileId, aFileId);
      do_check_eq(contact.alphaId, aContact.alphaId);
      do_check_eq(contact.number, aContact.number);
      do_check_eq(pin2, aPin2);
    };
    contactHelper.updateICCContact(CARD_APPTYPE_SIM, aContactType, aContact, aPin2);
  };

  let contact = {recordId: 1, alphaId: "test", number: "123456"};
  do_test(contact, "ADN", ICC_EF_ADN);
  do_test(contact, "FDN", ICC_EF_FDN, "1111");

  run_next_test();
});

