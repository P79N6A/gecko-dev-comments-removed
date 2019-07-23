





































gTestfile = '15.7.3.js';

















var SECTION = "15.7.3";
var VERSION = "ECMA_2";
startTest();
var TITLE   = "Properties of the Number Constructor";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase(SECTION,
	     "Number.__proto__",  
	     Function.prototype,
	     Number.__proto__ );

test();
