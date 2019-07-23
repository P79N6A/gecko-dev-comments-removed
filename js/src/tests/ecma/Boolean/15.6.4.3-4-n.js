





































gTestfile = '15.6.4.3-4-n.js';














var SECTION = "15.6.4.3-4-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Boolean.prototype.valueOf()";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "valof=Boolean.prototype.valueOf; x=new String( 'hello' ); x.valueOf=valof;x.valueOf()"
  EXPECTED = "error";

new TestCase(   SECTION,
		"valof=Boolean.prototype.valueOf; x=new String( 'hello' ); x.valueOf=valof;x.valueOf()",
		"error",
		eval("valof=Boolean.prototype.valueOf; x=new String( 'hello' ); x.valueOf=valof;x.valueOf()") );

test();
