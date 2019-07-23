





































gTestfile = 'in-001.js';












var SECTION = "in-001";
var VERSION = "JS1_3";
var TITLE   = "Regression test for 196109";
var BUGNUMBER="196109";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

o = {};
o.foo = 'sil';

new TestCase(
  SECTION,
  "\"foo\" in o",
  true,
  "foo" in o );

test();

