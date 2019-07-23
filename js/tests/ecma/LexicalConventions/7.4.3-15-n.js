





































gTestfile = '7.4.3-15-n.js';



















var SECTION = "7.4.3-15-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Future Reserved Words";

writeHeaderToLog( SECTION + " "+ TITLE);

var actual = 'no error';
var prefValue;

print("This test requires option javascript.options.strict enabled");

options('strict');
options('werror');

try
{
  eval("var import = true");
}
catch(e)
{
  actual = 'error';
}

DESCRIPTION = "var import = true";
EXPECTED = "error";


if (actual == 'error')
{
  throw actual;
}

new TestCase( SECTION, 
              "var import = true",    
              "error",   
              actual );

test();
