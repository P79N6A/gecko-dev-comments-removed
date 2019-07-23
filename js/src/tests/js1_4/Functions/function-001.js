





































gTestfile = 'function-001.js';
































var SECTION = "function-001.js";
var VERSION = "JS1_4";
var TITLE   = "Accessing the arguments property of a function object";
var BUGNUMBER="324455";
startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase(
  SECTION,
  "return function.arguments",
  "P",
  TestFunction_2("P", "A","S","S")[0] +"");


new TestCase(
  SECTION,
  "return arguments",
  "P",
  TestFunction_1( "P", "A", "S", "S" )[0] +"");

new TestCase(
  SECTION,
  "return arguments when function contains an arguments property",
  "PASS",
  TestFunction_3( "P", "A", "S", "S" ) +"");

new TestCase(
  SECTION,
  "return function.arguments when function contains an arguments property",
  "PASS",
  TestFunction_4( "F", "A", "I", "L" ) +"");

test();

function TestFunction_1( a, b, c, d, e ) {
  return arguments;
}

function TestFunction_2( a, b, c, d, e ) {
  return TestFunction_2.arguments;
}

function TestFunction_3( a, b, c, d, e ) {
  var arguments = "PASS";
  return arguments;
}

function TestFunction_4( a, b, c, d, e ) {
  var arguments = "PASS";
  return TestFunction_4.arguments;
}

