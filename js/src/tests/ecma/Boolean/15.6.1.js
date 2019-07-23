





































gTestfile = '15.6.1.js';






















var SECTION = "15.6.1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The Boolean constructor called as a function: Boolean( value ) and Boolean()";

writeHeaderToLog( SECTION + " "+ TITLE);

var array = new Array();
var item = 0;

new TestCase( SECTION,   "Boolean(1)",         true,   Boolean(1) );
new TestCase( SECTION,   "Boolean(0)",         false,  Boolean(0) );
new TestCase( SECTION,   "Boolean(-1)",        true,   Boolean(-1) );
new TestCase( SECTION,   "Boolean('1')",       true,   Boolean("1") );
new TestCase( SECTION,   "Boolean('0')",       true,   Boolean("0") );
new TestCase( SECTION,   "Boolean('-1')",      true,   Boolean("-1") );
new TestCase( SECTION,   "Boolean(true)",      true,   Boolean(true) );
new TestCase( SECTION,   "Boolean(false)",     false,  Boolean(false) );

new TestCase( SECTION,   "Boolean('true')",    true,   Boolean("true") );
new TestCase( SECTION,   "Boolean('false')",   true,   Boolean("false") );
new TestCase( SECTION,   "Boolean(null)",      false,  Boolean(null) );

new TestCase( SECTION,   "Boolean(-Infinity)", true,   Boolean(Number.NEGATIVE_INFINITY) );
new TestCase( SECTION,   "Boolean(NaN)",       false,  Boolean(Number.NaN) );
new TestCase( SECTION,   "Boolean(void(0))",   false,  Boolean( void(0) ) );
new TestCase( SECTION,   "Boolean(x=0)",       false,  Boolean( x=0 ) );
new TestCase( SECTION,   "Boolean(x=1)",       true,   Boolean( x=1 ) );
new TestCase( SECTION,   "Boolean(x=false)",   false,  Boolean( x=false ) );
new TestCase( SECTION,   "Boolean(x=true)",    true,   Boolean( x=true ) );
new TestCase( SECTION,   "Boolean(x=null)",    false,  Boolean( x=null ) );
new TestCase( SECTION,   "Boolean()",          false,  Boolean() );


test();
