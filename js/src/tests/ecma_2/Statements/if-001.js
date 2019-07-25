
















var SECTION = "for-001";
var VERSION = "ECMA_2";
var TITLE   = "The if  statement";
var BUGNUMBER="148822";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var a = 0;
var b = 0;
var result = "passed";

if ( a = b ) {
  result = "failed:  a = b should return 0";
}

new TestCase(
  SECTION,
  "if ( a = b ), where a and b are both equal to 0",
  "passed",
  result );


test();

