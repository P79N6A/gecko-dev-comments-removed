


const Cu = Components.utils;
const {Services} = Cu.import("resource://gre/modules/Services.jsm", {});

function run_test()
{
  let testFlag = Services.telemetry.getHistogramById("TELEMETRY_TEST_FLAG");
  equal(JSON.stringify(testFlag.snapshot().counts), "[1,0,0]", "Original value is correct");
  testFlag.add(1);
  equal(JSON.stringify(testFlag.snapshot().counts), "[0,1,0]", "Value is correct after ping.");
  testFlag.clear();
  equal(JSON.stringify(testFlag.snapshot().counts), "[1,0,0]", "Value is correct after calling clear()");
  testFlag.add(1);
  equal(JSON.stringify(testFlag.snapshot().counts), "[0,1,0]", "Value is correct after ping.");
}
