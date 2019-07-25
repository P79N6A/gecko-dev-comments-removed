














var SECTION = "";
var VERSION = "ECMA_2";
var TITLE   = "Keywords";

startTest();

print("This test requires option javascript.options.strict enabled");

if (!options().match(/strict/))
{
  options('strict');
}
if (!options().match(/werror/))
{
  options('werror');
}

var result = "failed";

try {
  eval("super;");
}
catch (x) {
  if (x instanceof SyntaxError)
    result = x.name;
}

AddTestCase(
  "using the expression \"super\" shouldn't cause js to crash",
  "SyntaxError",
  result );

test();
