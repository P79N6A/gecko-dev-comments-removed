





































gTestfile = '15.1.2.7.js';













var SECTION = "15.1.2.7";
var VERSION = "ECMA_1";
var TITLE   = "isFinite( x )";
var BUGNUMBER= "none";

startTest();

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION, "isFinite.length",      1,                  isFinite.length );
new TestCase( SECTION, "isFinite.length = null; isFinite.length",   1,      eval("isFinite.length=null; isFinite.length") );
new TestCase( SECTION, "delete isFinite.length",                    false,  delete isFinite.length );
new TestCase( SECTION, "delete isFinite.length; isFinite.length",   1,      eval("delete isFinite.length; isFinite.length") );
new TestCase( SECTION, "var MYPROPS=''; for ( p in isFinite ) { MYPROPS+= p }; MYPROPS",    "", eval("var MYPROPS=''; for ( p in isFinite ) { MYPROPS += p }; MYPROPS") );

new TestCase( SECTION,  "isFinite()",           false,              isFinite() );
new TestCase( SECTION, "isFinite( null )",      true,              isFinite(null) );
new TestCase( SECTION, "isFinite( void 0 )",    false,             isFinite(void 0) );
new TestCase( SECTION, "isFinite( false )",     true,              isFinite(false) );
new TestCase( SECTION, "isFinite( true)",       true,              isFinite(true) );
new TestCase( SECTION, "isFinite( ' ' )",       true,              isFinite( " " ) );

new TestCase( SECTION, "isFinite( new Boolean(true) )",     true,   isFinite(new Boolean(true)) );
new TestCase( SECTION, "isFinite( new Boolean(false) )",    true,   isFinite(new Boolean(false)) );

new TestCase( SECTION, "isFinite( 0 )",        true,              isFinite(0) );
new TestCase( SECTION, "isFinite( 1 )",        true,              isFinite(1) );
new TestCase( SECTION, "isFinite( 2 )",        true,              isFinite(2) );
new TestCase( SECTION, "isFinite( 3 )",        true,              isFinite(3) );
new TestCase( SECTION, "isFinite( 4 )",        true,              isFinite(4) );
new TestCase( SECTION, "isFinite( 5 )",        true,              isFinite(5) );
new TestCase( SECTION, "isFinite( 6 )",        true,              isFinite(6) );
new TestCase( SECTION, "isFinite( 7 )",        true,              isFinite(7) );
new TestCase( SECTION, "isFinite( 8 )",        true,              isFinite(8) );
new TestCase( SECTION, "isFinite( 9 )",        true,              isFinite(9) );

new TestCase( SECTION, "isFinite( '0' )",        true,              isFinite('0') );
new TestCase( SECTION, "isFinite( '1' )",        true,              isFinite('1') );
new TestCase( SECTION, "isFinite( '2' )",        true,              isFinite('2') );
new TestCase( SECTION, "isFinite( '3' )",        true,              isFinite('3') );
new TestCase( SECTION, "isFinite( '4' )",        true,              isFinite('4') );
new TestCase( SECTION, "isFinite( '5' )",        true,              isFinite('5') );
new TestCase( SECTION, "isFinite( '6' )",        true,              isFinite('6') );
new TestCase( SECTION, "isFinite( '7' )",        true,              isFinite('7') );
new TestCase( SECTION, "isFinite( '8' )",        true,              isFinite('8') );
new TestCase( SECTION, "isFinite( '9' )",        true,              isFinite('9') );

new TestCase( SECTION, "isFinite( 0x0a )",    true,                 isFinite( 0x0a ) );
new TestCase( SECTION, "isFinite( 0xaa )",    true,                 isFinite( 0xaa ) );
new TestCase( SECTION, "isFinite( 0x0A )",    true,                 isFinite( 0x0A ) );
new TestCase( SECTION, "isFinite( 0xAA )",    true,                 isFinite( 0xAA ) );

new TestCase( SECTION, "isFinite( '0x0a' )",    true,               isFinite( "0x0a" ) );
new TestCase( SECTION, "isFinite( '0xaa' )",    true,               isFinite( "0xaa" ) );
new TestCase( SECTION, "isFinite( '0x0A' )",    true,               isFinite( "0x0A" ) );
new TestCase( SECTION, "isFinite( '0xAA' )",    true,               isFinite( "0xAA" ) );

new TestCase( SECTION, "isFinite( 077 )",       true,               isFinite( 077 ) );
new TestCase( SECTION, "isFinite( '077' )",     true,               isFinite( "077" ) );

new TestCase( SECTION, "isFinite( new String('Infinity') )",        false,      isFinite(new String("Infinity")) );
new TestCase( SECTION, "isFinite( new String('-Infinity') )",       false,      isFinite(new String("-Infinity")) );

new TestCase( SECTION, "isFinite( 'Infinity' )",        false,      isFinite("Infinity") );
new TestCase( SECTION, "isFinite( '-Infinity' )",       false,      isFinite("-Infinity") );
new TestCase( SECTION, "isFinite( Number.POSITIVE_INFINITY )",  false,  isFinite(Number.POSITIVE_INFINITY) );
new TestCase( SECTION, "isFinite( Number.NEGATIVE_INFINITY )",  false,  isFinite(Number.NEGATIVE_INFINITY) );
new TestCase( SECTION, "isFinite( Number.NaN )",                false,  isFinite(Number.NaN) );

new TestCase( SECTION, "isFinite( Infinity )",  false,  isFinite(Infinity) );
new TestCase( SECTION, "isFinite( -Infinity )",  false,  isFinite(-Infinity) );
new TestCase( SECTION, "isFinite( NaN )",                false,  isFinite(NaN) );


new TestCase( SECTION, "isFinite( Number.MAX_VALUE )",          true,  isFinite(Number.MAX_VALUE) );
new TestCase( SECTION, "isFinite( Number.MIN_VALUE )",          true,  isFinite(Number.MIN_VALUE) );

test();
