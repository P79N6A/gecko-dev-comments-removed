





































gTestfile = '15.5.4.2-1.js';


















var SECTION = "15.5.4.2-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.prototype.toString";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,   "String.prototype.toString()",        "",     String.prototype.toString() );
new TestCase( SECTION,   "(new String()).toString()",          "",     (new String()).toString() );
new TestCase( SECTION,   "(new String(\"\")).toString()",      "",     (new String("")).toString() );
new TestCase( SECTION,   "(new String( String() )).toString()","",    (new String(String())).toString() );
new TestCase( SECTION,  "(new String( \"h e l l o\" )).toString()",       "h e l l o",    (new String("h e l l o")).toString() );
new TestCase( SECTION,   "(new String( 0 )).toString()",       "0",    (new String(0)).toString() );

test();
