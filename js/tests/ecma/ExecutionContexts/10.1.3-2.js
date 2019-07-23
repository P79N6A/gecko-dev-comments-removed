





































gTestfile = '10.1.3-2.js';










var SECTION = "10.1.3-2";
var VERSION = "ECMA_1";
var TITLE   = "Variable Instantiation:  Function Declarations";
var BUGNUMBER="299639";
startTest();

writeHeaderToLog( SECTION + " "+ TITLE);

function f(g)
{
  function g() {
    return "g";
  };
  return g;
}

new TestCase(
  SECTION,
  "typeof f(\"parameter\")",
  "function",
  typeof f("parameter") );

test();

