





































gTestfile = '11.2.2-1.js';












































var SECTION = "11.2.2-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The new operator";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
              "(new TestFunction(0,1,2,3,4,5)).length",
              6,
              (new TestFunction(0,1,2,3,4,5)).length );

test();

function TestFunction() {
  return arguments;
}
