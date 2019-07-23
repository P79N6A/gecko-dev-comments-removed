





































gTestfile = 'function-002.js';













var SECTION = "function-002";
var VERSION = "JS1_3";
var TITLE   = "Regression test for 249579";
var BUGNUMBER="249579";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase(
  SECTION,
  "0?function(){}:0",
  0,
  0?function(){}:0 );


bar = true;
foo = bar ? function () { return true; } : function() { return false; };

new TestCase(
  SECTION,
  "bar = true; foo = bar ? function () { return true; } : function() { return false; }; foo()",
  true,
  foo() );


test();

