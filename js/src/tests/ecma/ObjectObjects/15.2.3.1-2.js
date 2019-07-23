





































gTestfile = '15.2.3.1-2.js';


















var SECTION = "15.2.3.1-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Object.prototype";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION, 
	      "delete( Object.prototype )",
	      false,
	      eval("delete( Object.prototype )") );

test();
