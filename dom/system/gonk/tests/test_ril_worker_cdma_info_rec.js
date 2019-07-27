


subscriptLoader.loadSubScript("resource://gre/modules/ril_consts.js", this);

function run_test() {
  run_next_test();
}




function newWorkerWithParcel(parcelBuf) {
  let worker = newWorker({
    postRILMessage: function(data) {
      
    },
    postMessage: function(message) {
      
    }
  });

  let index = 0; 
  let buf = parcelBuf;

  let context = worker.ContextPool._contexts[0];
  context.Buf.readUint8 = function() {
    return buf[index++];
  };

  context.Buf.readUint16 = function() {
    return buf[index++];
  };

  context.Buf.readInt32 = function() {
    return buf[index++];
  };

  context.Buf.seekIncoming = function(offset) {
    index += offset / context.Buf.UINT32_SIZE;
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
  let context = worker.ContextPool._contexts[0];
  let helper = context.CdmaPDUHelper;
  let records = helper.decodeInformationRecord();

  equal(records[0].display, "Test Info");

  run_next_test();
});




add_test(function test_extended_display() {
  let worker = newWorkerWithParcel([
                0x01, 
                0x07, 
                0x12, 
                0x54, 0x65, 0x73, 0x74, 0x20, 0x45, 0x78, 0x74,
                0x65, 0x6E, 0x64, 0x65, 0x64, 0x20, 0x49, 0x6E,
                0x66, 0x6F, 0x00, 0x00]);
  let context = worker.ContextPool._contexts[0];
  let helper = context.CdmaPDUHelper;
  let records = helper.decodeInformationRecord();

  equal(records[0].display, "Test Extended Info");

  run_next_test();
});




add_test(function test_mixed() {
  let worker = newWorkerWithParcel([
                0x02, 
                0x00, 
                0x0B, 
                0x54, 0x65, 0x73, 0x74, 0x20, 0x49, 0x6E, 0x66,
                0x6F, 0x20, 0x31, 0x00,
                0x07, 
                0x0B, 
                0x54, 0x65, 0x73, 0x74, 0x20, 0x49, 0x6E, 0x66,
                0x6F, 0x20, 0x32, 0x00]);
  let context = worker.ContextPool._contexts[0];
  let helper = context.CdmaPDUHelper;
  let records = helper.decodeInformationRecord();

  equal(records[0].display, "Test Info 1");
  equal(records[1].display, "Test Info 2");

  run_next_test();
});




add_test(function test_multiple() {
  let worker = newWorkerWithParcel([
                0x02, 
                0x00, 
                0x0B, 
                0x54, 0x65, 0x73, 0x74, 0x20, 0x49, 0x6E, 0x66,
                0x6F, 0x20, 0x31, 0x00,
                0x00, 
                0x0B, 
                0x54, 0x65, 0x73, 0x74, 0x20, 0x49, 0x6E, 0x66,
                0x6F, 0x20, 0x32, 0x00]);
  let context = worker.ContextPool._contexts[0];
  let helper = context.CdmaPDUHelper;
  let records = helper.decodeInformationRecord();

  equal(records[0].display, "Test Info 1");
  equal(records[1].display, "Test Info 2");

  run_next_test();
});




add_test(function test_signal() {
  let worker = newWorkerWithParcel([
                0x01,   
                0x04,   
                0x01,   
                0x00,   
                0x01,   
                0x03]); 
  let context = worker.ContextPool._contexts[0];
  let helper = context.CdmaPDUHelper;
  let records = helper.decodeInformationRecord();

  equal(records[0].signal.type, 0x00);
  equal(records[0].signal.alertPitch, 0x01);
  equal(records[0].signal.signal, 0x03);

  run_next_test();
});




add_test(function test_signal_not_present() {
  let worker = newWorkerWithParcel([
                0x01,   
                0x04,   
                0x00,   
                0x00,   
                0x01,   
                0x03]); 
  let context = worker.ContextPool._contexts[0];
  let helper = context.CdmaPDUHelper;
  let records = helper.decodeInformationRecord();

  equal(records.length, 0);

  run_next_test();
});




add_test(function test_line_control() {
  let worker = newWorkerWithParcel([
                0x01,   
                0x06,   
                0x01,   
                0x00,   
                0x01,   
                0xFF]); 
  let context = worker.ContextPool._contexts[0];
  let helper = context.CdmaPDUHelper;
  let records = helper.decodeInformationRecord();

  equal(records[0].lineControl.polarityIncluded, 1);
  equal(records[0].lineControl.toggle, 0);
  equal(records[0].lineControl.reverse, 1);
  equal(records[0].lineControl.powerDenial, 255);

  run_next_test();
});




add_test(function test_clir() {
  let worker = newWorkerWithParcel([
                0x01,   
                0x08,   
                0x01]); 
  let context = worker.ContextPool._contexts[0];
  let helper = context.CdmaPDUHelper;
  let records = helper.decodeInformationRecord();

  equal(records[0].clirCause, 1);

  run_next_test();
});




add_test(function test_clir() {
  let worker = newWorkerWithParcel([
                0x01,   
                0x0A,   
                0x01,   
                0xFF]); 
  let context = worker.ContextPool._contexts[0];
  let helper = context.CdmaPDUHelper;
  let records = helper.decodeInformationRecord();

  equal(records[0].audioControl.upLink, 1);
  equal(records[0].audioControl.downLink, 255);

  run_next_test();
});
