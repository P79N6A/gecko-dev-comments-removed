






















































var SECTION = "7.4.3-9-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Future Reserved Words";

writeHeaderToLog( SECTION + " "+ TITLE);

var actual = 'no error';
var prefValue;

DESCRIPTION = "var class = true";
EXPECTED = "error";


print("This test requires option javascript.options.strict enabled");
var jsOptions = new JavaScriptOptions();
jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);

try
{
  eval("var class = true");
}
catch(e)
{
  actual = 'error';
}

jsOptions.reset();


if (actual == 'error')
{
  throw actual;
}

new TestCase( SECTION,  
              "var class = true",     
              "error",    
              actual );

test();

