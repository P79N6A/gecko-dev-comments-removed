





































gTestfile = '15.5.4.5-1.js';































var SECTION = "15.5.4.5-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.prototype.charCodeAt";

writeHeaderToLog( SECTION + " "+ TITLE);

var TEST_STRING = new String( " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" );

for ( j = 0, i = 0x0020; i < 0x007e; i++, j++ ) {
  new TestCase( SECTION, "TEST_STRING.charCodeAt("+j+")", i, TEST_STRING.charCodeAt( j ) );
}

new TestCase( SECTION, 'TEST_STRING.charCodeAt('+i+')', NaN,    TEST_STRING.charCodeAt( i ) );


test();
