





































gTestfile = '12.6.1-1.js';











var SECTION = "12.6.1-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The While statement";
writeHeaderToLog( SECTION + " "+ TITLE);


new TestCase( SECTION,
	      "var MYVAR = 0; while( MYVAR++ < 100) { if ( MYVAR < 100 ) break; } MYVAR ",
	      1,
	      eval("var MYVAR = 0; while( MYVAR++ < 100) { if ( MYVAR < 100 ) break; } MYVAR "));

new TestCase( SECTION,
	      "var MYVAR = 0; while( MYVAR++ < 100) { if ( MYVAR < 100 ) continue; else break; } MYVAR ",
	      100,
	      eval("var MYVAR = 0; while( MYVAR++ < 100) { if ( MYVAR < 100 ) continue; else break; } MYVAR "));

new TestCase( SECTION,
	      "function MYFUN( arg1 ) { while ( arg1++ < 100 ) { if ( arg1 < 100 ) return arg1; } }; MYFUN(1)",
	      2,
	      eval("function MYFUN( arg1 ) { while ( arg1++ < 100 ) { if ( arg1 < 100 ) return arg1; } }; MYFUN(1)"));

test();

