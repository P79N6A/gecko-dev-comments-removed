





































gTestfile = '15.5.3.1-2.js';

















var SECTION = "15.5.3.1-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Properties of the String Constructor";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "String.prototype=null;String.prototype",
	      String.prototype,
	      eval("String.prototype=null;String.prototype") );

test();
