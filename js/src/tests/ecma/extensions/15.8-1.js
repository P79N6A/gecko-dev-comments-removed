





































gTestfile = '15.8-1.js';



























var SECTION = "15.8-1";
var VERSION = "ECMA_2";
startTest();
var TITLE   = "The Math Object";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Math.__proto__ == Object.prototype",
	      true,
	      Math.__proto__ == Object.prototype );

new TestCase( SECTION,
	      "Math.__proto__",
	      Object.prototype,
	      Math.__proto__ );

test();
