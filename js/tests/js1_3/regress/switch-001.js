





































gTestfile = 'switch-001.js';
















var SECTION = "switch-001";
var VERSION = "JS1_3";
var TITLE   = "switch-001";
var BUGNUMBER="315767";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

result = "fail:  did not enter switch";

switch (true) {
case 1:
  result = "fail:  version 130 should force strict equality";
  break;
case true:
  result = "pass";
  break;
default:
  result = "fail: evaluated default statement";
}

new TestCase(
  SECTION,
  "switch / case should use strict equality in version of JS < 1.4",
  "pass",
  result );

test();

