













var SECTION = "lexical-049";
var VERSION = "JS1_4";
var TITLE   = "The Rules of Automatic Semicolon Insertion";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  var counter = 0;
  eval("for ( counter = 0\n"
       + "counter <= 1;\n"
       + "counter++ )\n"
       + "{\n"
       + "result += \": got inside for loop\";\n"
       + "}\n");

} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "line breaks within a for expression" +
  " (threw " + exception +")",
  expect,
  result );

test();


