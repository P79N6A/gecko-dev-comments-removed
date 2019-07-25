















var SECTION = "lexical-051";
var VERSION = "JS1_4";
var TITLE   = "Examples of Automatic Semicolon Insertion";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  eval("for (a; b\n) result += \": got to inner loop\";")
    } catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "for (a; b\n)" +
  " (threw " + exception +")",
  expect,
  result );

test();



