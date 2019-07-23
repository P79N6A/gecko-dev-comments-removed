






















































var SECTION = "7.4.3-14-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Future Reserved Words";

writeHeaderToLog( SECTION + " "+ TITLE);

var actual = 'no error';
var prefValue;

print("This test requires option javascript.options.strict enabled");
var jsOptions = new JavaScriptOptions();
jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);

try
{
  eval("var enum = true");
}
catch(e)
{
  actual = 'error';
}

jsOptions.reset();

DESCRIPTION = "var enum = true";
EXPECTED = "error";


if (actual == 'error')
{
  throw actual;
}

new TestCase( SECTION,  
              "var enum = true",     
              "error",    
              actual );

test();


