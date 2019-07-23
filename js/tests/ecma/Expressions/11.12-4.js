





































gTestfile = '11.12-4.js';


















var SECTION = "11.12-4";
var VERSION = "ECMA_1";
startTest();
writeHeaderToLog( SECTION + " Conditional operator ( ? : )");



new TestCase( SECTION,
	      "true ? MYVAR1 = 'PASSED' : MYVAR1 = 'FAILED'; MYVAR1",
	      "PASSED",
	      eval("true ? MYVAR1 = 'PASSED' : MYVAR1 = 'FAILED'; MYVAR1") );

test();

