









var SECTION = "";
var VERSION = "ECMA_2";
var TITLE   = "Keywords";

startTest();

print("This test requires option javascript.options.strict enabled");

var jsOptions = new JavaScriptOptions();
jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);

var result = "failed";

try {
  eval("super;");
} 
catch (x) {
  if (x instanceof SyntaxError)
    result = x.name;
}

jsOptions.reset();

AddTestCase(
  "using the expression \"super\" shouldn't cause js to crash",
  "SyntaxError",
  result );

test();
