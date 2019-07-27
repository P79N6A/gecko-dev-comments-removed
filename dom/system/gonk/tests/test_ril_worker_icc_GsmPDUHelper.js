


subscriptLoader.loadSubScript("resource://gre/modules/ril_consts.js", this);

function run_test() {
  run_next_test();
}




add_test(function test_write_timestamp() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let helper = context.GsmPDUHelper;

  
  let dateInput = new Date();
  let dateOutput = new Date();
  helper.writeTimestamp(dateInput);
  dateOutput.setTime(helper.readTimestamp());

  equal(dateInput.getFullYear(), dateOutput.getFullYear());
  equal(dateInput.getMonth(), dateOutput.getMonth());
  equal(dateInput.getDate(), dateOutput.getDate());
  equal(dateInput.getHours(), dateOutput.getHours());
  equal(dateInput.getMinutes(), dateOutput.getMinutes());
  equal(dateInput.getSeconds(), dateOutput.getSeconds());
  equal(dateInput.getTimezoneOffset(), dateOutput.getTimezoneOffset());

  
  let time = Date.UTC(2034, 1, 23, 12, 34, 56);
  time = time - (8 * 60 * 60 * 1000);
  dateInput.setTime(time);
  helper.writeTimestamp(dateInput);
  dateOutput.setTime(helper.readTimestamp());

  equal(dateInput.getFullYear(), dateOutput.getFullYear());
  equal(dateInput.getMonth(), dateOutput.getMonth());
  equal(dateInput.getDate(), dateOutput.getDate());
  equal(dateInput.getHours(), dateOutput.getHours());
  equal(dateInput.getMinutes(), dateOutput.getMinutes());
  equal(dateInput.getSeconds(), dateOutput.getSeconds());
  equal(dateInput.getTimezoneOffset(), dateOutput.getTimezoneOffset());

  run_next_test();
});




add_test(function test_octect_BCD() {
  let worker = newUint8Worker();
  let context = worker.ContextPool._contexts[0];
  let helper = context.GsmPDUHelper;

  
  let number = 23;
  let octet = helper.BCDToOctet(number);
  equal(helper.octetToBCD(octet), number);

  
  number = 56;
  octet = helper.BCDToOctet(number);
  equal(helper.octetToBCD(octet), number);

  
  octet = 0x23;
  number = helper.octetToBCD(octet);
  equal(helper.BCDToOctet(number), octet);

  
  octet = 0x56;
  number = helper.octetToBCD(octet);
  equal(helper.BCDToOctet(number), octet);

  run_next_test();
});
