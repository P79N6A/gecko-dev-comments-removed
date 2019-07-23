





































gTestfile = '15.2.3.1-1.js';

















var SECTION = "15.2.3.1-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Object.prototype";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION, 
	      "var str = '';for ( p in Object ) { str += p; }; str",
	      "",
	      eval( "var str = ''; for ( p in Object ) { str += p; }; str" ) );

test();
