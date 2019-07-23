





































gTestfile = '11.14-1.js';























var SECTION = "11.14-1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Comma operator (,)");

new TestCase( SECTION,    "true, false",                    false,  eval("true, false") );
new TestCase( SECTION,    "VAR1=true, VAR2=false",          false,  eval("VAR1=true, VAR2=false") );
new TestCase( SECTION,    "VAR1=true, VAR2=false;VAR1",     true,   eval("VAR1=true, VAR2=false; VAR1") );

test();
