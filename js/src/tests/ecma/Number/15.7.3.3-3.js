





































gTestfile = '15.7.3.3-3.js';












var SECTION = "15.7.3.3-3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Number.MIN_VALUE:  ReadOnly Attribute";

writeHeaderToLog( SECTION + " "+TITLE );

new TestCase( SECTION,
	      "Number.MIN_VALUE=0; Number.MIN_VALUE",
	      Number.MIN_VALUE,
	      eval("Number.MIN_VALUE=0; Number.MIN_VALUE" ));

test();
