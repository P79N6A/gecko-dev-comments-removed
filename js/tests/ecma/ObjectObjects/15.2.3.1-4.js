






















































var SECTION = "15.2.3.1-4";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Object.prototype";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,  
	      "delete( Object.prototype ); Object.prototype",
	      Object.prototype,
	      eval("delete(Object.prototype); Object.prototype") );

test();
