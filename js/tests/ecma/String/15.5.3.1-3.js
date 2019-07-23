





































gTestfile = '15.5.3.1-3.js';

















var SECTION = "15.5.3.1-3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Properties of the String Constructor";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,	"delete( String.prototype )",   false,   eval("delete ( String.prototype )") );

test();
