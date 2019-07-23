





































gTestfile = 'array-007-n.js';










var SECTION = "LiveConnect";
var VERSION = "1_3";
var TITLE   = "Java Array to JavaScript JavaArray object";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);




var E_TYPE = "object";
var E_CLASS = "[object JavaArray]";

var byte_array = ( new java.lang.String("hi") ).getBytes();

byte_array.name = "name";

DESCRIPTION = "byte_array.name = \"name\"; byte_array.name";
EXPECTED = "error";

new TestCase(
  SECTION,
  "byte_array.name = \"name\"; byte_array.name",
  void 0,
  byte_array.name );

byte_array["0"] = 127;

test();

