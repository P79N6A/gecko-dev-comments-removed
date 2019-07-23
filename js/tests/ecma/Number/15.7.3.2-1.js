





































gTestfile = '15.7.3.2-1.js';













var SECTION = "15.7.3.2-1";
var VERSION = "ECMA_1";
startTest();
var TITLE =  "Number.MAX_VALUE";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Number.MAX_VALUE",     
	      1.7976931348623157e308,    
	      Number.MAX_VALUE );

test();
