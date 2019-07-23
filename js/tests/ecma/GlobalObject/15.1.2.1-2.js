





































gTestfile = '15.1.2.1-2.js';













var SECTION = "15.1.2.1-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "eval(x)";
var BUGNUMBER = "none";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase(    SECTION,
		 "d = new Date(0); with (d) { x = getUTCMonth() +'/'+ getUTCDate() +'/'+ getUTCFullYear(); } x",
		 "0/1/1970",
		 eval( "d = new Date(0); with (d) { x = getUTCMonth() +'/'+ getUTCDate() +'/'+ getUTCFullYear(); } x" ));

test();
