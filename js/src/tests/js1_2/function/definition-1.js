





































gTestfile = 'definition-1.js';










var SECTION = "function/definition-1.js";
var VERSION = "JS_12";
startTest();
var TITLE   = "Regression test for 111284";

writeHeaderToLog( SECTION + " "+ TITLE);

f1 = function() { return "passed!" }

  function f2() { f3 = function() { return "passed!" }; return f3(); }

new TestCase( SECTION,
	      'f1 = function() { return "passed!" }; f1()',
	      "passed!",
	      f1() );

new TestCase( SECTION,
	      'function f2() { f3 = function { return "passed!" }; return f3() }; f2()',
	      "passed!",
	      f2() );

new TestCase( SECTION,
	      'f3()',
	      "passed!",
	      f3() );

test();

