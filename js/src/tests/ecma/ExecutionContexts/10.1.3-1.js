





































gTestfile = '10.1.3-1.js';
























var SECTION = "10.1.3-1";
var VERSION = "ECMA_1";
var TITLE   = "Variable Instantiation:  Formal Parameters";
var BUGNUMBER="104191";
startTest();

writeHeaderToLog( SECTION + " "+ TITLE);

var myfun1 = new Function( "a", "a", "return a" );
var myfun2 = new Function( "a", "b", "a", "return a" );

function myfun3(a, b, a) {
  return a;
}




new TestCase(
  SECTION,
  String(myfun2) +"; myfun2(2,4,8)",
  8,
  myfun2(2,4,8) );

new TestCase(
  SECTION,
  "myfun2(2,4)",
  void 0,
  myfun2(2,4));

new TestCase(
  SECTION,
  String(myfun3) +"; myfun3(2,4,8)",
  8,
  myfun3(2,4,8) );

new TestCase(
  SECTION,
  "myfun3(2,4)",
  void 0,
  myfun3(2,4) );

test();

