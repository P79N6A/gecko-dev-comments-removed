





































gTestfile = 'new-001.js';











var SECTION = "new-001";
var VERSION = "JS1_3";
var TITLE   = "new-001";
var BUGNUMBER="31567";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

function Test_One (x) {
  this.v = x+1;
  return x*2
    }

function Test_Two( x, y ) {
  this.v = x;
  return y;
}

new TestCase(
  SECTION,
  "Test_One(18)",
  36,
  Test_One(18) );

new TestCase(
  SECTION,
  "new Test_One(18)",
  "[object Object]",
  new Test_One(18) +"" );

new TestCase(
  SECTION,
  "new Test_One(18).v",
  19,
  new Test_One(18).v );

new TestCase(
  SECTION,
  "Test_Two(2,7)",
  7,
  Test_Two(2,7) );

new TestCase(
  SECTION,
  "new Test_Two(2,7)",
  "[object Object]",
  new Test_Two(2,7) +"" );

new TestCase(
  SECTION,
  "new Test_Two(2,7).v",
  2,
  new Test_Two(2,7).v );

new TestCase(
  SECTION,
  "new (Function)(\"x\", \"return x+3\")(5,6)",
  8,
  new (Function)("x","return x+3")(5,6) );

new TestCase(
  SECTION,
  "new new Test_Two(String, 2).v(0123)",
  "83",
  new new Test_Two(String, 2).v(0123) +"");

new TestCase(
  SECTION,
  "new new Test_Two(String, 2).v(0123).length",
  2,
  new new Test_Two(String, 2).v(0123).length );

test();
