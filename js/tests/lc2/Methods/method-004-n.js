





































gTestfile = 'method-004-n.js';











var SECTION = "LiveConnect Objects";
var VERSION = "1_3";
var TITLE   = "Passing bad arguments to a method";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var string = new java.lang.String(\"\"); string.charAt(\"foo\")";
EXPECTED = "error";

var string = new java.lang.String("");

new TestCase(
  SECTION,
  "var string = new java.lang.String(\"\"); string.charAt(\"foo\")",
  "error",
  string.charAt("foo") );

test();

