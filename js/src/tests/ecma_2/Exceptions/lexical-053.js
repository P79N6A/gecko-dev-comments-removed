















var SECTION = "lexical-053";
var VERSION = "JS1_4";
var TITLE   = "Examples of Automatic Semicolon Insertion";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  a = true
    b = false

    eval('if (a > b)\nelse result += ": got to else statement"');
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "calling return indirectly" +
  " (threw " + exception +")",
  expect,
  result );

test();
