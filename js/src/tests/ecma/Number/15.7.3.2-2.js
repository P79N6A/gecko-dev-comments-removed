





































gTestfile = '15.7.3.2-2.js';













var SECTION = "15.7.3.2-2";
var VERSION = "ECMA_1";
startTest();
var TITLE =  "Number.MAX_VALUE:  DontDelete Attribute";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "delete( Number.MAX_VALUE ); Number.MAX_VALUE",     
	      1.7976931348623157e308,
	      eval("delete( Number.MAX_VALUE );Number.MAX_VALUE") );

new TestCase( SECTION,
	      "delete( Number.MAX_VALUE )", 
	      false, 
	      eval("delete( Number.MAX_VALUE )") );

test();
