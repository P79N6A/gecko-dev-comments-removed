





































gTestfile = '15.5.4.5-2.js';































var SECTION = "15.5.4.5-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.prototype.charCodeAt";

writeHeaderToLog( SECTION + " "+ TITLE);

var TEST_STRING = new String( " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" );

var x;

new TestCase( SECTION,     "x = new Boolean(true); x.charCodeAt=String.prototype.charCodeAt;x.charCodeAt(0)", 0x0074,    eval("x = new Boolean(true); x.charCodeAt=String.prototype.charCodeAt;x.charCodeAt(0)") );
new TestCase( SECTION,     "x = new Boolean(true); x.charCodeAt=String.prototype.charCodeAt;x.charCodeAt(1)", 0x0072,    eval("x = new Boolean(true); x.charCodeAt=String.prototype.charCodeAt;x.charCodeAt(1)") );
new TestCase( SECTION,     "x = new Boolean(true); x.charCodeAt=String.prototype.charCodeAt;x.charCodeAt(2)", 0x0075,    eval("x = new Boolean(true); x.charCodeAt=String.prototype.charCodeAt;x.charCodeAt(2)") );
new TestCase( SECTION,     "x = new Boolean(true); x.charCodeAt=String.prototype.charCodeAt;x.charCodeAt(3)", 0x0065,    eval("x = new Boolean(true); x.charCodeAt=String.prototype.charCodeAt;x.charCodeAt(3)") );
new TestCase( SECTION,     "x = new Boolean(true); x.charCodeAt=String.prototype.charCodeAt;x.charCodeAt(4)", Number.NaN,     eval("x = new Boolean(true); x.charCodeAt=String.prototype.charCodeAt;x.charCodeAt(4)") );
new TestCase( SECTION,     "x = new Boolean(true); x.charCodeAt=String.prototype.charCodeAt;x.charCodeAt(-1)", Number.NaN,    eval("x = new Boolean(true); x.charCodeAt=String.prototype.charCodeAt;x.charCodeAt(-1)") );

new TestCase( SECTION,     "x = new Boolean(true); x.charCodeAt=String.prototype.charCodeAt;x.charCodeAt(true)",  0x0072,    eval("x = new Boolean(true); x.charCodeAt=String.prototype.charCodeAt;x.charCodeAt(true)") );
new TestCase( SECTION,     "x = new Boolean(true); x.charCodeAt=String.prototype.charCodeAt;x.charCodeAt(false)", 0x0074,    eval("x = new Boolean(true); x.charCodeAt=String.prototype.charCodeAt;x.charCodeAt(false)") );

new TestCase( SECTION,     "x = new String(); x.charCodeAt(0)",    Number.NaN,     eval("x=new String();x.charCodeAt(0)") );
new TestCase( SECTION,     "x = new String(); x.charCodeAt(1)",    Number.NaN,     eval("x=new String();x.charCodeAt(1)") );
new TestCase( SECTION,     "x = new String(); x.charCodeAt(-1)",   Number.NaN,     eval("x=new String();x.charCodeAt(-1)") );

new TestCase( SECTION,     "x = new String(); x.charCodeAt(NaN)",                       Number.NaN,     eval("x=new String();x.charCodeAt(Number.NaN)") );
new TestCase( SECTION,     "x = new String(); x.charCodeAt(Number.POSITIVE_INFINITY)",  Number.NaN,     eval("x=new String();x.charCodeAt(Number.POSITIVE_INFINITY)") );
new TestCase( SECTION,     "x = new String(); x.charCodeAt(Number.NEGATIVE_INFINITY)",  Number.NaN,     eval("x=new String();x.charCodeAt(Number.NEGATIVE_INFINITY)") );

new TestCase( SECTION,  "x = new Array(1,2,3); x.charCodeAt = String.prototype.charCodeAt; x.charCodeAt(0)",    0x0031,   eval("x = new Array(1,2,3); x.charCodeAt = String.prototype.charCodeAt; x.charCodeAt(0)") );
new TestCase( SECTION,  "x = new Array(1,2,3); x.charCodeAt = String.prototype.charCodeAt; x.charCodeAt(1)",    0x002C,   eval("x = new Array(1,2,3); x.charCodeAt = String.prototype.charCodeAt; x.charCodeAt(1)") );
new TestCase( SECTION,  "x = new Array(1,2,3); x.charCodeAt = String.prototype.charCodeAt; x.charCodeAt(2)",    0x0032,   eval("x = new Array(1,2,3); x.charCodeAt = String.prototype.charCodeAt; x.charCodeAt(2)") );
new TestCase( SECTION,  "x = new Array(1,2,3); x.charCodeAt = String.prototype.charCodeAt; x.charCodeAt(3)",    0x002C,   eval("x = new Array(1,2,3); x.charCodeAt = String.prototype.charCodeAt; x.charCodeAt(3)") );
new TestCase( SECTION,  "x = new Array(1,2,3); x.charCodeAt = String.prototype.charCodeAt; x.charCodeAt(4)",    0x0033,   eval("x = new Array(1,2,3); x.charCodeAt = String.prototype.charCodeAt; x.charCodeAt(4)") );
new TestCase( SECTION,  "x = new Array(1,2,3); x.charCodeAt = String.prototype.charCodeAt; x.charCodeAt(5)",    NaN,   eval("x = new Array(1,2,3); x.charCodeAt = String.prototype.charCodeAt; x.charCodeAt(5)") );

new TestCase( SECTION,  "x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(0)", 0x005B, eval("x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(0)") );
new TestCase( SECTION,  "x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(1)", 0x006F, eval("x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(1)") );
new TestCase( SECTION,  "x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(2)", 0x0062, eval("x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(2)") );
new TestCase( SECTION,  "x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(3)", 0x006A, eval("x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(3)") );
new TestCase( SECTION,  "x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(4)", 0x0065, eval("x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(4)") );
new TestCase( SECTION,  "x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(5)", 0x0063, eval("x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(5)") );
new TestCase( SECTION,  "x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(6)", 0x0074, eval("x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(6)") );

new TestCase( SECTION,  "x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(7)", 0x0020, eval("x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(7)") );

new TestCase( SECTION,  "x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(8)", 0x004F, eval("x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(8)") );
new TestCase( SECTION,  "x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(9)", 0x0062, eval("x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(9)") );
new TestCase( SECTION,  "x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(10)", 0x006A, eval("x = new Function( 'this.charCodeAt = String.prototype.charCodeAt' ); f = new x(); f.charCodeAt(10)") );

test();
