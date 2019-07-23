





































gTestfile = '15.6.4.3-3.js';















var SECTION = "15.6.4.3-3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Boolean.prototype.valueOf()";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "x=true; x.valueOf=Boolean.prototype.valueOf;x.valueOf()",
	      true,
	      eval("x=true; x.valueOf=Boolean.prototype.valueOf;x.valueOf()") );
test();
