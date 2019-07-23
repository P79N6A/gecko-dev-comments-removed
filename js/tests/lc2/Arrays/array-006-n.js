




































 








var SECTION = "LiveConnect";
var VERSION = "1_3";
var TITLE   = "Java Array to JavaScript JavaArray object";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);




var E_TYPE = "object";
var E_CLASS = "[object JavaArray]";

var byte_array = ( new java.lang.String("hi") ).getBytes();

DESCRIPTION = "byte_array[\"foo\"]";
EXPECTED = "error";

new TestCase(
    SECTION,
    "byte_array[\"foo\"]",
    void 0,
    byte_array["foo"] );

test();

