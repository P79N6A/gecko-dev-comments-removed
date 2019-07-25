















var SECTION = "lexical-003.js";
var VERSION = "JS1_4";
var TITLE   = "Comments";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  eval("/*\n/* nested comment */\n*/\n");
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "/*/*nested comment*/ */" +
  " (threw " + exception +")",
  expect,
  result );

test();

