





































gTestfile = '15.3.5.3.js';





















var SECTION = "15.3.5.3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Function.arguments";

writeHeaderToLog( SECTION + " "+ TITLE);

var MYFUNCTION = new Function( "return this.arguments" );

new TestCase( SECTION,  "var MYFUNCTION = new Function( 'return this.arguments' ); MYFUNCTION.arguments",   null,   MYFUNCTION.arguments );

test();
