





































gTestfile = 'nesting-1.js';










var SECTION = "function/nesting-1.js";
var VERSION = "JS_12";
startTest();
var TITLE   = "Regression test for 122040";

writeHeaderToLog( SECTION + " "+ TITLE);

function f(a) {function g(b) {return a+b;}; return g;}; f(7);

new TestCase( SECTION,
	      'function f(a) {function g(b) {return a+b;}; return g;}; typeof f(7)',
	      "function",
	      typeof f(7) );

test();

