





































gTestfile = '15.2.3.1-3.js';


















var SECTION = "15.2.3.1-3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Object.prototype";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION, 
	      "Object.prototype = null; Object.prototype",
	      Object.prototype,
	      eval("Object.prototype = null; Object.prototype"));

test();
