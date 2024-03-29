
























var SECTION = "lexical-011";
var VERSION = "JS1_4";
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
  eval("super = true;");
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "super = true" +
  " (threw " + exception +")",
  expect,
  result );

test();
