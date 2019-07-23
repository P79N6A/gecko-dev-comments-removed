





































gTestfile = 'function-001.js';














var SECTION = "function-001.js";
var VERSION = "JS1_4";
var TITLE   = "Regression test case for 325843";
var BUGNUMBER="3258435";
startTest();

writeHeaderToLog( SECTION + " "+ TITLE);

eval("function f1 (a){ var a,b; }");

function f2( a ) { var a, b; };

new TestCase(
  SECTION,
  "eval(\"function f1 (a){ var a,b; }\"); "+
  "function f2( a ) { var a, b; }; typeof f1",
  "function",
  typeof f1 );



new TestCase(
  SECTION,
  "typeof f1.toString()",
  "string",
  typeof f1.toString() );

new TestCase(
  SECTION,
  "typeof f2",
  "function",
  typeof f2 );



new TestCase(
  SECTION,
  "typeof f2.toString()",
  "string",
  typeof f2.toString() );

test();

