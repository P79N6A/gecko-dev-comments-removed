





































gTestfile = '15.5.3.1-4.js';

















var SECTION = "15.5.3.1-4";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Properties of the String Constructor";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,	"delete( String.prototype );String.prototype",   String.prototype,   eval("delete ( String.prototype );String.prototype") );

test();
