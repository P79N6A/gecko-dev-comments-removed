





































gTestfile = '15.6.4.2-4-n.js';















var SECTION = "15.6.4.2-4-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Boolean.prototype.toString()";
writeHeaderToLog( SECTION +" "+ TITLE );

DESCRIPTION = "tostr=Boolean.prototype.toString; x=new String( 'hello' ); x.toString=tostr; x.toString()";
EXPECTED = "error";

new TestCase(   SECTION,
		"tostr=Boolean.prototype.toString; x=new String( 'hello' ); x.toString=tostr; x.toString()",
		"error",
		eval("tostr=Boolean.prototype.toString; x=new String( 'hello' ); x.toString=tostr; x.toString()") );

test();
