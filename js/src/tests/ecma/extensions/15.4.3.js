





































gTestfile = '15.4.3.js';











var SECTION = "15.4.3";
var VERSION = "ECMA_2";
startTest();
var TITLE   = "Properties of the Array Constructor";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Array.__proto__",     
	      Function.prototype,       
	      Array.__proto__ );

test();
