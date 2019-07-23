





































gTestfile = '12.5-1.js';























var SECTION = "12.5-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The if statement";

writeHeaderToLog( SECTION + " "+ TITLE);


new TestCase(   SECTION,
		"var MYVAR; if ( true ) MYVAR='PASSED'; else MYVAR= 'FAILED';",
		"PASSED",
		eval("var MYVAR; if ( true ) MYVAR='PASSED'; else MYVAR= 'FAILED';") );

new TestCase(  SECTION,
	       "var MYVAR; if ( false ) MYVAR='FAILED'; else MYVAR= 'PASSED';",
	       "PASSED",
	       eval("var MYVAR; if ( false ) MYVAR='FAILED'; else MYVAR= 'PASSED';") );

new TestCase(   SECTION,
		"var MYVAR; if ( new Boolean(true) ) MYVAR='PASSED'; else MYVAR= 'FAILED';",
		"PASSED",
		eval("var MYVAR; if ( new Boolean(true) ) MYVAR='PASSED'; else MYVAR= 'FAILED';") );

new TestCase(  SECTION,
	       "var MYVAR; if ( new Boolean(false) ) MYVAR='PASSED'; else MYVAR= 'FAILED';",
	       "PASSED",
	       eval("var MYVAR; if ( new Boolean(false) ) MYVAR='PASSED'; else MYVAR= 'FAILED';") );

new TestCase(   SECTION,
		"var MYVAR; if ( 1 ) MYVAR='PASSED'; else MYVAR= 'FAILED';",
		"PASSED",
		eval("var MYVAR; if ( 1 ) MYVAR='PASSED'; else MYVAR= 'FAILED';") );

new TestCase(  SECTION,
	       "var MYVAR; if ( 0 ) MYVAR='FAILED'; else MYVAR= 'PASSED';",
	       "PASSED",
	       eval("var MYVAR; if ( 0 ) MYVAR='FAILED'; else MYVAR= 'PASSED';") );

test();

