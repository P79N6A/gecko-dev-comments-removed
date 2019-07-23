





































gTestfile = 'string-006.js';








var SECTION = "Preferred argument conversion: string";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
  SECTION;
startTest();

var TEST_CLASS = new
  Packages.com.netscape.javascript.qa.lc3.string.String_001;

var string = "255";

new TestCase(
  "TEST_CLASS[\"ambiguous(java.lang.String)\"](string))",
  "STRING",
  TEST_CLASS["ambiguous(java.lang.String)"](string) +'');

new TestCase(
  "TEST_CLASS[\"ambiguous(java.lang.Object)\"](string))",
  "OBJECT",
  TEST_CLASS["ambiguous(java.lang.Object)"](string) +'');

new TestCase(
  "TEST_CLASS[\"ambiguous(char)\"](string))",
  "CHAR",
  TEST_CLASS["ambiguous(char)"](string) +'');

new TestCase(
  "TEST_CLASS[\"ambiguous(double)\"](string))",
  "DOUBLE",
  TEST_CLASS["ambiguous(double)"](string) +'');

new TestCase(
  "TEST_CLASS[\"ambiguous(float)\"](string))",
  "FLOAT",
  TEST_CLASS["ambiguous(float)"](string) +'');

new TestCase(
  "TEST_CLASS[\"ambiguous(long)\"](string))",
  "LONG",
  TEST_CLASS["ambiguous(long)"](string) +'');

new TestCase(
  "TEST_CLASS[\"ambiguous(int)\"](string))",
  "INT",
  TEST_CLASS["ambiguous(int)"](string) +'');

new TestCase(
  "TEST_CLASS[\"ambiguous(short)\"](string))",
  "SHORT",
  TEST_CLASS["ambiguous(short)"](string) +'');

new TestCase(
  "TEST_CLASS[\"ambiguous(byte)\"](\"127\"))",
  "BYTE",
  TEST_CLASS["ambiguous(byte)"]("127") +'');


test();
