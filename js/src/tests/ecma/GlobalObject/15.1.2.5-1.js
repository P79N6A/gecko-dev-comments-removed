





































gTestfile = '15.1.2.5-1.js';














































var SECTION = "15.1.2.5-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "unescape(string)";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION, "unescape.length",       1,               unescape.length );
new TestCase( SECTION, "unescape.length = null; unescape.length",   1,      eval("unescape.length=null; unescape.length") );
new TestCase( SECTION, "delete unescape.length",                    false,  delete unescape.length );
new TestCase( SECTION, "delete unescape.length; unescape.length",   1,      eval("delete unescape.length; unescape.length") );
new TestCase( SECTION, "var MYPROPS=''; for ( var p in unescape ) { MYPROPS+= p }; MYPROPS",    "", eval("var MYPROPS=''; for ( var p in unescape ) { MYPROPS+= p }; MYPROPS") );

new TestCase( SECTION, "unescape()",              "undefined",    unescape() );
new TestCase( SECTION, "unescape('')",            "",             unescape('') );
new TestCase( SECTION, "unescape( null )",        "null",         unescape(null) );
new TestCase( SECTION, "unescape( void 0 )",      "undefined",    unescape(void 0) );
new TestCase( SECTION, "unescape( true )",        "true",         unescape( true ) );
new TestCase( SECTION, "unescape( false )",       "false",        unescape( false ) );

new TestCase( SECTION, "unescape( new Boolean(true) )",   "true", unescape(new Boolean(true)) );
new TestCase( SECTION, "unescape( new Boolean(false) )",  "false",    unescape(new Boolean(false)) );

new TestCase( SECTION, "unescape( Number.NaN  )",                 "NaN",      unescape(Number.NaN) );
new TestCase( SECTION, "unescape( -0 )",                          "0",        unescape( -0 ) );
new TestCase( SECTION, "unescape( 'Infinity' )",                  "Infinity", unescape( "Infinity" ) );
new TestCase( SECTION, "unescape( Number.POSITIVE_INFINITY )",    "Infinity", unescape( Number.POSITIVE_INFINITY ) );
new TestCase( SECTION, "unescape( Number.NEGATIVE_INFINITY )",    "-Infinity", unescape( Number.NEGATIVE_INFINITY ) );

var ASCII_TEST_STRING = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789@*_+-./";

new TestCase( SECTION, "unescape( " +ASCII_TEST_STRING+" )",    ASCII_TEST_STRING,  unescape( ASCII_TEST_STRING ) );



for ( var CHARCODE = 0; CHARCODE < 256; CHARCODE++ ) {
  new TestCase( SECTION,
		"unescape( %"+ ToHexString(CHARCODE)+" )",
		String.fromCharCode(CHARCODE),
		unescape( "%" + ToHexString(CHARCODE) )  );
}


for ( var CHARCODE = 0; CHARCODE < 256; CHARCODE++ ) {
  new TestCase( SECTION,
		"unescape( %u"+ ToHexString(CHARCODE)+" )",
		"%u"+ToHexString(CHARCODE),
		unescape( "%u" + ToHexString(CHARCODE) )  );
}















test();

function ToUnicodeString( n ) {
  var string = ToHexString(n);

  for ( var PAD = (4 - string.length ); PAD > 0; PAD-- ) {
    string = "0" + string;
  }

  return string;
}
function ToHexString( n ) {
  var hex = new Array();

  for ( var mag = 1; Math.pow(16,mag) <= n ; mag++ ) {
    ;
  }

  for ( index = 0, mag -= 1; mag > 0; index++, mag-- ) {
    hex[index] = Math.floor( n / Math.pow(16,mag) );
    n -= Math.pow(16,mag) * Math.floor( n/Math.pow(16,mag) );
  }

  hex[hex.length] = n % 16;

  var string ="";

  for ( var index = 0 ; index < hex.length ; index++ ) {
    switch ( hex[index] ) {
    case 10:
      string += "A";
      break;
    case 11:
      string += "B";
      break;
    case 12:
      string += "C";
      break;
    case 13:
      string += "D";
      break;
    case 14:
      string += "E";
      break;
    case 15:
      string += "F";
      break;
    default:
      string += hex[index];
    }
  }

  if ( string.length == 1 ) {
    string = "0" + string;
  }
  return string;
}
