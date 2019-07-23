




































gTestfile = 'lexical-022.js';




















var SECTION = "lexical-022.js";
var VERSION = "ECMA_1";
var TITLE   = "Future Reserved Words";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  eval("import = true;");
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "import = true" +
  " (threw " + exception +")",
  expect,
  result );

test();


