





































gTestfile = '11.12-3.js';


















var SECTION = "11.12-3";
var VERSION = "ECMA_1";
startTest();
writeHeaderToLog( SECTION + " Conditional operator ( ? : )");



new TestCase( SECTION,
	      "var MYVAR =  true ? ('FAIL1', 'PASSED') : 'FAIL2'; MYVAR",
	      "PASSED",
	      eval("var MYVAR =  true ? ('FAIL1', 'PASSED') : 'FAIL2'; MYVAR"));

test();

