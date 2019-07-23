





































gTestfile = '11.9.2.js';


















var SECTION = "11.9.2";
var VERSION = "ECMA_1";
var BUGNUMBER="77391";
startTest();

writeHeaderToLog( SECTION + " The equals operator ( == )");



new TestCase( SECTION,    "void 0 == void 0",        false,   void 0 != void 0 );
new TestCase( SECTION,    "null == null",           false,   null != null );



new TestCase( SECTION,    "NaN != NaN",             true,  Number.NaN != Number.NaN );
new TestCase( SECTION,    "NaN != 0",               true,  Number.NaN != 0 );
new TestCase( SECTION,    "0 != NaN",               true,  0 != Number.NaN );
new TestCase( SECTION,    "NaN != Infinity",        true,  Number.NaN != Number.POSITIVE_INFINITY );
new TestCase( SECTION,    "Infinity != NaN",        true,  Number.POSITIVE_INFINITY != Number.NaN );



new TestCase( SECTION,    "Number.MAX_VALUE != Number.MAX_VALUE",   false,   Number.MAX_VALUE != Number.MAX_VALUE );
new TestCase( SECTION,    "Number.MIN_VALUE != Number.MIN_VALUE",   false,   Number.MIN_VALUE != Number.MIN_VALUE );
new TestCase( SECTION,    "Number.POSITIVE_INFINITY != Number.POSITIVE_INFINITY",   false,   Number.POSITIVE_INFINITY != Number.POSITIVE_INFINITY );
new TestCase( SECTION,    "Number.NEGATIVE_INFINITY != Number.NEGATIVE_INFINITY",   false,   Number.NEGATIVE_INFINITY != Number.NEGATIVE_INFINITY );



new TestCase( SECTION,    "0 != 0",                 false,   0 != 0 );
new TestCase( SECTION,    "0 != -0",                false,   0 != -0 );
new TestCase( SECTION,    "-0 != 0",                false,   -0 != 0 );
new TestCase( SECTION,    "-0 != -0",               false,   -0 != -0 );



new TestCase( SECTION,    "0.9 != 1",               true,  0.9 != 1 );
new TestCase( SECTION,    "0.999999 != 1",          true,  0.999999 != 1 );
new TestCase( SECTION,    "0.9999999999 != 1",      true,  0.9999999999 != 1 );
new TestCase( SECTION,    "0.9999999999999 != 1",   true,  0.9999999999999 != 1 );







new TestCase( SECTION,    "'hello' != 'hello'",         false,   "hello" != "hello" );



new TestCase( SECTION,    "true != true",               false,   true != true );
new TestCase( SECTION,    "false != false",             false,   false != false );
new TestCase( SECTION,    "true != false",              true,  true != false );
new TestCase( SECTION,    "false != true",              true,  false != true );



new TestCase( SECTION,    "new MyObject(true) != new MyObject(true)",   true,  new MyObject(true) != new MyObject(true) );
new TestCase( SECTION,    "new Boolean(true) != new Boolean(true)",     true,  new Boolean(true) != new Boolean(true) );
new TestCase( SECTION,    "new Boolean(false) != new Boolean(false)",   true,  new Boolean(false) != new Boolean(false) );


new TestCase( SECTION,    "x = new MyObject(true); y = x; z = x; z != y",   false,  eval("x = new MyObject(true); y = x; z = x; z != y") );
new TestCase( SECTION,    "x = new MyObject(false); y = x; z = x; z != y",  false,  eval("x = new MyObject(false); y = x; z = x; z != y") );
new TestCase( SECTION,    "x = new Boolean(true); y = x; z = x; z != y",   false,  eval("x = new Boolean(true); y = x; z = x; z != y") );
new TestCase( SECTION,    "x = new Boolean(false); y = x; z = x; z != y",   false,  eval("x = new Boolean(false); y = x; z = x; z != y") );

new TestCase( SECTION,    "new Boolean(true) != new Boolean(true)",     true,  new Boolean(true) != new Boolean(true) );
new TestCase( SECTION,    "new Boolean(false) != new Boolean(false)",   true,  new Boolean(false) != new Boolean(false) );



new TestCase( SECTION,    "null != void 0",             false,   null != void 0 );
new TestCase( SECTION,    "void 0 != null",             false,   void 0 != null );



new TestCase( SECTION,    "1 != '1'",                   false,   1 != '1' );
new TestCase( SECTION,    "255 != '0xff'",               false,  255 != '0xff' );
new TestCase( SECTION,    "0 != '\r'",                  false,   0 != "\r" );
new TestCase( SECTION,    "1e19 != '1e19'",             false,   1e19 != "1e19" );


new TestCase( SECTION,    "new Boolean(true) != true",  false,   true != new Boolean(true) );
new TestCase( SECTION,    "new MyObject(true) != true", false,   true != new MyObject(true) );

new TestCase( SECTION,    "new Boolean(false) != false",    false,   new Boolean(false) != false );
new TestCase( SECTION,    "new MyObject(false) != false",   false,   new MyObject(false) != false );

new TestCase( SECTION,    "true != new Boolean(true)",      false,   true != new Boolean(true) );
new TestCase( SECTION,    "true != new MyObject(true)",     false,   true != new MyObject(true) );

new TestCase( SECTION,    "false != new Boolean(false)",    false,   false != new Boolean(false) );
new TestCase( SECTION,    "false != new MyObject(false)",   false,   false != new MyObject(false) );

test();

function MyObject( value ) {
  this.value = value;
  this.valueOf = new Function( "return this.value" );
}
