



















var SECTION = "lexical-016";
var VERSION = "JS1_4";
var TITLE   = "Future Reserved Words";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

print("This test requires option javascript.options.strict enabled");

var jsOptions = new JavaScriptOptions();
jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  eval("class = true;");
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

jsOptions.reset();

new TestCase(
  SECTION,
  "class = true" +
  " (threw " + exception +")",
  expect,
  result );

test();
