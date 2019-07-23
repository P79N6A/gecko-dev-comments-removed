





































gTestfile = '7.4.3-16-n.js';



















var SECTION = "lexical-023.js";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Future Reserved Words";

writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";









DESCRIPTION = "try = true";
EXPECTED = "error";

new TestCase(
  SECTION,
  "try = true" +
  " (threw " + exception +")",
  "error",
  eval("try = true") );

test();
