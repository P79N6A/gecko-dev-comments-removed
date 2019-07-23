





































gTestfile = 'byte-002.js';







var SECTION = "java array object inheritance JavaScript Array methods";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 " + SECTION;

startTest();

var b = new java.lang.String("abcdefghijklmnopqrstuvwxyz").getBytes();

new TestCase(
  "var b = new java.lang.String(\"abcdefghijklmnopqrstuvwxyz\").getBytes(); b.valueOf()",
  b,
  b.valueOf() );

test();
