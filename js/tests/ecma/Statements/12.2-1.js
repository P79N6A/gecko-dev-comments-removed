





































gTestfile = '12.2-1.js';





















var SECTION = "12.2-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The variable statement";

writeHeaderToLog( SECTION +" "+ TITLE);

new TestCase(    "SECTION",
		 "var x = 3; function f() { var a = x; var x = 23; return a; }; f()",
		 void 0,
		 eval("var x = 3; function f() { var a = x; var x = 23; return a; }; f()") );

test();

