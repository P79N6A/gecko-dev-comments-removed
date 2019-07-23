





































gTestfile = 'array-008-n.js';
















var SECTION = "LiveConnect";
var VERSION = "1_3";
var TITLE   = "Java Array to JavaScript JavaArray object";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

dt = new Packages.com.netscape.javascript.qa.liveconnect.DataTypeClass;

var ba_length = dt.PUB_ARRAY_BYTE.length;

DESCRIPTION = "dt.PUB_ARRAY_BYTE.length = "+ ba_length;
EXPECTED = "error";

new TestCase(
  SECTION,
  "dt.PUB_ARRAY_BYTE.length = "+ ba_length,
  "error",
  dt.PUB_ARRAY_BYTE[ba_length] );

test();
