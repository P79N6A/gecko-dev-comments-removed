





































gTestfile = 'eval-001.js';






























var SECTION = "eval-001.js";
var VERSION = "JS1_4";
var TITLE   = "Calling eval indirectly should NOT fail in version 140";
var BUGNUMBER="38512";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var MY_EVAL = eval;
var RESULT = "";
var EXPECT = "abcdefg";

MY_EVAL( "RESULT = EXPECT" );

new TestCase(
  SECTION,
  "Call eval indirectly",
  EXPECT,
  RESULT );

test();

