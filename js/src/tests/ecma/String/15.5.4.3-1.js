





































gTestfile = '15.5.4.3-1.js';
















var SECTION = "15.5.4.3-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.prototype.valueOf";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,   "String.prototype.valueOf.length", 0,      String.prototype.valueOf.length );

new TestCase( SECTION,   "String.prototype.valueOf()",        "",     String.prototype.valueOf() );
new TestCase( SECTION,   "(new String()).valueOf()",          "",     (new String()).valueOf() );
new TestCase( SECTION,   "(new String(\"\")).valueOf()",      "",     (new String("")).valueOf() );
new TestCase( SECTION,   "(new String( String() )).valueOf()","",    (new String(String())).valueOf() );
new TestCase( SECTION,   "(new String( \"h e l l o\" )).valueOf()",       "h e l l o",    (new String("h e l l o")).valueOf() );
new TestCase( SECTION,   "(new String( 0 )).valueOf()",       "0",    (new String(0)).valueOf() );

test();
