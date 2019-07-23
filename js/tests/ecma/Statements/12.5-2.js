





































gTestfile = '12.5-2.js';






















var SECTION = "12.5-2";
var VERSION = "ECMA_1";
startTest();
var TITLE = "The if statement" ;

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase(   SECTION,
		"var MYVAR; if ( true ) MYVAR='PASSED'; MYVAR",
		"PASSED",
		eval("var MYVAR; if ( true ) MYVAR='PASSED'; MYVAR") );

new TestCase(  SECTION,
	       "var MYVAR; if ( false ) MYVAR='FAILED'; MYVAR;",
	       "PASSED",
	       eval("var MYVAR=\"PASSED\"; if ( false ) MYVAR='FAILED'; MYVAR;") );

new TestCase(   SECTION,
		"var MYVAR; if ( new Boolean(true) ) MYVAR='PASSED'; MYVAR",
		"PASSED",
		eval("var MYVAR; if ( new Boolean(true) ) MYVAR='PASSED'; MYVAR") );

new TestCase(   SECTION,
		"var MYVAR; if ( new Boolean(false) ) MYVAR='PASSED'; MYVAR",
		"PASSED",
		eval("var MYVAR; if ( new Boolean(false) ) MYVAR='PASSED'; MYVAR") );

new TestCase(   SECTION,
		"var MYVAR; if ( 1 ) MYVAR='PASSED'; MYVAR",
		"PASSED",
		eval("var MYVAR; if ( 1 ) MYVAR='PASSED'; MYVAR") );

new TestCase(  SECTION,
	       "var MYVAR; if ( 0 ) MYVAR='FAILED'; MYVAR;",
	       "PASSED",
	       eval("var MYVAR=\"PASSED\"; if ( 0 ) MYVAR='FAILED'; MYVAR;") );

test();
