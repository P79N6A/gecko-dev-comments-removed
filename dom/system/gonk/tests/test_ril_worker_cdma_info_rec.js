


subscriptLoader.loadSubScript("resource://gre/modules/ril_consts.js", this);

function run_test() {
  run_next_test();
}




function newWorkerWithParcel(parcelBuf) {
  let worker = newWorker({
    postRILMessage: function fakePostRILMessage(data) {
      
    },
    postMessage: function fakePostMessage(message) {
      
    }
  });

  let index = 0; 
  let buf = parcelBuf;

  worker.Buf.readUint8 = function () {
    return buf[index++];
  };

  worker.Buf.readUint16 = function () {
    return buf[index++];
  };

  worker.Buf.readInt32 = function () {
    return buf[index++];
  };

  return worker;
}






add_test(function test_display() {
  let worker = newWorkerWithParcel([
                0x01, 
                0x00, 
                0x09, 
                0x54, 0x65, 0x73, 0x74, 0x20, 0x49, 0x6E, 0x66,
                0x6F, 0x00]);
  let helper = worker.CdmaPDUHelper;
  let record = helper.decodeInformationRecord();

  do_check_eq(record.display, "Test Info");

  run_next_test();
});




add_test(function test_extended_display() {
  let worker = newWorkerWithParcel([
                0x01, 
                0x07, 
                0x0E, 
                0x80, 
                0x80, 
                0x81, 
                0x9B, 
                0x09, 0x54, 0x65, 0x73, 0x74, 0x20, 0x49, 0x6E,
                0x66, 0x6F, 0x00]);
  let helper = worker.CdmaPDUHelper;
  let record = helper.decodeInformationRecord();

  do_check_eq(record.extendedDisplay.indicator, 1);
  do_check_eq(record.extendedDisplay.type, 0);
  do_check_eq(record.extendedDisplay.records.length, 3);
  do_check_eq(record.extendedDisplay.records[0].tag, 0x80);
  do_check_eq(record.extendedDisplay.records[1].tag, 0x81);
  do_check_eq(record.extendedDisplay.records[2].tag, 0x9B);
  do_check_eq(record.extendedDisplay.records[2].content, "Test Info");

  run_next_test();
});



add_test(function test_mixed() {
  let worker = newWorkerWithParcel([
                0x02, 
                0x00, 
                0x09, 
                0x54, 0x65, 0x73, 0x74, 0x20, 0x49, 0x6E, 0x66,
                0x6F, 0x00,
                0x07, 
                0x0E, 
                0x80, 
                0x80, 
                0x81, 
                0x9B, 
                0x09, 0x54, 0x65, 0x73, 0x74, 0x20, 0x49, 0x6E,
                0x66, 0x6F, 0x00]);
  let helper = worker.CdmaPDUHelper;
  let record = helper.decodeInformationRecord();

  do_check_eq(record.display, "Test Info");
  do_check_eq(record.extendedDisplay.indicator, 1);
  do_check_eq(record.extendedDisplay.type, 0);
  do_check_eq(record.extendedDisplay.records.length, 3);
  do_check_eq(record.extendedDisplay.records[0].tag, 0x80);
  do_check_eq(record.extendedDisplay.records[1].tag, 0x81);
  do_check_eq(record.extendedDisplay.records[2].tag, 0x9B);
  do_check_eq(record.extendedDisplay.records[2].content, "Test Info");

  run_next_test();
});
