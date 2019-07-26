


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




add_test(function test_read_8bit_unpacked_to_string() {
  let worker = newUint8Worker();
  let helper = worker.GsmPDUHelper;
  let buf = worker.Buf;
  const langTable = PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];

  
  for (let i = 0; i < PDU_NL_EXTENDED_ESCAPE; i++) {
    helper.writeHexOctet(i);
  }

  
  let ffLen = 2;
  for (let i = 0; i < ffLen; i++) {
    helper.writeHexOctet(0xff);
  }

  do_check_eq(helper.read8BitUnpackedToString(PDU_NL_EXTENDED_ESCAPE + ffLen),
              langTable.substring(0, PDU_NL_EXTENDED_ESCAPE));
  run_next_test();
});




add_test(function test_is_icc_service_available() {
  let worker = newUint8Worker();

  function test_table(sst, geckoService, simEnabled, usimEnabled) {
    worker.RIL.iccInfo.sst = sst;
    worker.RIL.appType = CARD_APPTYPE_SIM;
    do_check_eq(worker.RIL.isICCServiceAvailable(geckoService), simEnabled);
    worker.RIL.appType = CARD_APPTYPE_USIM;
    do_check_eq(worker.RIL.isICCServiceAvailable(geckoService), usimEnabled);
  }

  test_table([0x08], "ADN", true, false);
  test_table([0x08], "FDN", false, false);
  test_table([0x08], "SDN", false, true);

  run_next_test();
});
