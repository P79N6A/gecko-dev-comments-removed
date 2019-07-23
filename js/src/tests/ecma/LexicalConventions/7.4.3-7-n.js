





































gTestfile = '7.4.3-7-n.js';



















var SECTION = "7.4.3-7-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Future Reserved Words";

writeHeaderToLog( SECTION + " "+ TITLE);

var actual = 'no error';
var prefValue;

print("This test requires option javascript.options.strict enabled");

if (!options().match(/strict/))
{
  options('strict');
}
if (!options().match(/werror/))
{
  options('werror');
}

try
{
  eval("var extends = true");
}
catch(e)
{
  actual = 'error';
}

DESCRIPTION = "var extends = true";
EXPECTED = "error";


if (actual == 'error')
{
  throw actual;
}

new TestCase( SECTION, 
              "var extends = true",    
              "error",   
              actual);

test();
