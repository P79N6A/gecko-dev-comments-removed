





































gTestfile = '15.6.3.js';















var SECTION = "15.6.3";
var VERSION = "ECMA_2";
startTest();
var TITLE   = "Properties of the Boolean Constructor"
  writeHeaderToLog( SECTION + TITLE );


new TestCase( SECTION,  "Boolean.__proto__ == Function.prototype",  true,   Boolean.__proto__ == Function.prototype );
new TestCase( SECTION,  "Boolean.length",          1,                   Boolean.length );

test();
