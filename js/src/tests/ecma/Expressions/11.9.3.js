





































gTestfile = '11.9.3.js';


















var SECTION = "11.9.3";
var VERSION = "ECMA_1";
var BUGNUMBER="77391";
startTest();

writeHeaderToLog( SECTION + " The equals operator ( == )");



new TestCase( SECTION,    "void 0 = void 0",        true,   void 0 == void 0 );
new TestCase( SECTION,    "null == null",           true,   null == null );



new TestCase( SECTION,    "NaN == NaN",             false,  Number.NaN == Number.NaN );
new TestCase( SECTION,    "NaN == 0",               false,  Number.NaN == 0 );
new TestCase( SECTION,    "0 == NaN",               false,  0 == Number.NaN );
new TestCase( SECTION,    "NaN == Infinity",        false,  Number.NaN == Number.POSITIVE_INFINITY );
new TestCase( SECTION,    "Infinity == NaN",        false,  Number.POSITIVE_INFINITY == Number.NaN );



new TestCase( SECTION,    "Number.MAX_VALUE == Number.MAX_VALUE",   true,   Number.MAX_VALUE == Number.MAX_VALUE );
new TestCase( SECTION,    "Number.MIN_VALUE == Number.MIN_VALUE",   true,   Number.MIN_VALUE == Number.MIN_VALUE );
new TestCase( SECTION,    "Number.POSITIVE_INFINITY == Number.POSITIVE_INFINITY",   true,   Number.POSITIVE_INFINITY == Number.POSITIVE_INFINITY );
new TestCase( SECTION,    "Number.NEGATIVE_INFINITY == Number.NEGATIVE_INFINITY",   true,   Number.NEGATIVE_INFINITY == Number.NEGATIVE_INFINITY );



new TestCase( SECTION,    "0 == 0",                 true,   0 == 0 );
new TestCase( SECTION,    "0 == -0",                true,   0 == -0 );
new TestCase( SECTION,    "-0 == 0",                true,   -0 == 0 );
new TestCase( SECTION,    "-0 == -0",               true,   -0 == -0 );



new TestCase( SECTION,    "0.9 == 1",               false,  0.9 == 1 );
new TestCase( SECTION,    "0.999999 == 1",          false,  0.999999 == 1 );
new TestCase( SECTION,    "0.9999999999 == 1",      false,  0.9999999999 == 1 );
new TestCase( SECTION,    "0.9999999999999 == 1",   false,  0.9999999999999 == 1 );







new TestCase( SECTION,    "'hello' == 'hello'",         true,   "hello" == "hello" );



new TestCase( SECTION,    "true == true",               true,   true == true );
new TestCase( SECTION,    "false == false",             true,   false == false );
new TestCase( SECTION,    "true == false",              false,  true == false );
new TestCase( SECTION,    "false == true",              false,  false == true );



new TestCase( SECTION,    "new MyObject(true) == new MyObject(true)",   false,  new MyObject(true) == new MyObject(true) );
new TestCase( SECTION,    "new Boolean(true) == new Boolean(true)",     false,  new Boolean(true) == new Boolean(true) );
new TestCase( SECTION,    "new Boolean(false) == new Boolean(false)",   false,  new Boolean(false) == new Boolean(false) );


new TestCase( SECTION,    "x = new MyObject(true); y = x; z = x; z == y",   true,  eval("x = new MyObject(true); y = x; z = x; z == y") );
new TestCase( SECTION,    "x = new MyObject(false); y = x; z = x; z == y",  true,  eval("x = new MyObject(false); y = x; z = x; z == y") );
new TestCase( SECTION,    "x = new Boolean(true); y = x; z = x; z == y",   true,  eval("x = new Boolean(true); y = x; z = x; z == y") );
new TestCase( SECTION,    "x = new Boolean(false); y = x; z = x; z == y",   true,  eval("x = new Boolean(false); y = x; z = x; z == y") );

new TestCase( SECTION,    "new Boolean(true) == new Boolean(true)",     false,  new Boolean(true) == new Boolean(true) );
new TestCase( SECTION,    "new Boolean(false) == new Boolean(false)",   false,  new Boolean(false) == new Boolean(false) );



new TestCase( SECTION,    "null == void 0",             true,   null == void 0 );
new TestCase( SECTION,    "void 0 == null",             true,   void 0 == null );



new TestCase( SECTION,    "1 == '1'",                   true,   1 == '1' );
new TestCase( SECTION,    "255 == '0xff'",               true,  255 == '0xff' );
new TestCase( SECTION,    "0 == '\r'",                  true,   0 == "\r" );
new TestCase( SECTION,    "1e19 == '1e19'",             true,   1e19 == "1e19" );


new TestCase( SECTION,    "new Boolean(true) == true",  true,   true == new Boolean(true) );
new TestCase( SECTION,    "new MyObject(true) == true", true,   true == new MyObject(true) );

new TestCase( SECTION,    "new Boolean(false) == false",    true,   new Boolean(false) == false );
new TestCase( SECTION,    "new MyObject(false) == false",   true,   new MyObject(false) == false );

new TestCase( SECTION,    "true == new Boolean(true)",      true,   true == new Boolean(true) );
new TestCase( SECTION,    "true == new MyObject(true)",     true,   true == new MyObject(true) );

new TestCase( SECTION,    "false == new Boolean(false)",    true,   false == new Boolean(false) );
new TestCase( SECTION,    "false == new MyObject(false)",   true,   false == new MyObject(false) );

test();

function MyObject( value ) {
  this.value = value;
  this.valueOf = new Function( "return this.value" );
}
