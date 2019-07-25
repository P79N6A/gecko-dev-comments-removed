
























var SECTION = "lexical-021.js";
var VERSION = "ECMA_1";
var TITLE   = "Future Reserved Words";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

print("This test requires option javascript.options.strict enabled");

if (!options().match(/strict/))
{
  options('strict');
}
if (!options().match(/werror/))
{
  options('werror');
}

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  eval("enum = true;");
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "enum = true" +
  " (threw " + exception +")",
  expect,
  result );

test();
