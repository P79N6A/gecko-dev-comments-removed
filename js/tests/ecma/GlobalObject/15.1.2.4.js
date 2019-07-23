





































gTestfile = '15.1.2.4.js';
















































var SECTION = "15.1.2.4";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "escape(string)";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION, "escape.length",         1,          escape.length );
new TestCase( SECTION, "escape.length = null; escape.length",   1,  eval("escape.length = null; escape.length") );
new TestCase( SECTION, "delete escape.length",                  false,  delete escape.length );
new TestCase( SECTION, "delete escape.length; escape.length",   1,      eval("delete escape.length; escape.length") );
new TestCase( SECTION, "var MYPROPS=''; for ( var p in escape ) { MYPROPS+= p}; MYPROPS",    "",    eval("var MYPROPS=''; for ( var p in escape ) { MYPROPS+= p}; MYPROPS") );

new TestCase( SECTION, "escape()",              "undefined",    escape() );
new TestCase( SECTION, "escape('')",            "",             escape('') );
new TestCase( SECTION, "escape( null )",        "null",         escape(null) );
new TestCase( SECTION, "escape( void 0 )",      "undefined",    escape(void 0) );
new TestCase( SECTION, "escape( true )",        "true",         escape( true ) );
new TestCase( SECTION, "escape( false )",       "false",        escape( false ) );

new TestCase( SECTION, "escape( new Boolean(true) )",   "true", escape(new Boolean(true)) );
new TestCase( SECTION, "escape( new Boolean(false) )",  "false",    escape(new Boolean(false)) );

new TestCase( SECTION, "escape( Number.NaN  )",                 "NaN",      escape(Number.NaN) );
new TestCase( SECTION, "escape( -0 )",                          "0",        escape( -0 ) );
new TestCase( SECTION, "escape( 'Infinity' )",                  "Infinity", escape( "Infinity" ) );
new TestCase( SECTION, "escape( Number.POSITIVE_INFINITY )",    "Infinity", escape( Number.POSITIVE_INFINITY ) );
new TestCase( SECTION, "escape( Number.NEGATIVE_INFINITY )",    "-Infinity", escape( Number.NEGATIVE_INFINITY ) );

var ASCII_TEST_STRING = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789@*_+-./";

new TestCase( SECTION, "escape( " +ASCII_TEST_STRING+" )",    ASCII_TEST_STRING,  escape( ASCII_TEST_STRING ) );



for ( var CHARCODE = 0; CHARCODE < 32; CHARCODE++ ) {
  new TestCase( SECTION,
		"escape(String.fromCharCode("+CHARCODE+"))",
		"%"+ToHexString(CHARCODE),
		escape(String.fromCharCode(CHARCODE))  );
}
for ( var CHARCODE = 128; CHARCODE < 256; CHARCODE++ ) {
  new TestCase( SECTION,
		"escape(String.fromCharCode("+CHARCODE+"))",
		"%"+ToHexString(CHARCODE),
		escape(String.fromCharCode(CHARCODE))  );
}

for ( var CHARCODE = 256; CHARCODE < 1024; CHARCODE++ ) {
  new TestCase( SECTION,
		"escape(String.fromCharCode("+CHARCODE+"))",
		"%u"+ ToUnicodeString(CHARCODE),
		escape(String.fromCharCode(CHARCODE))  );
}
for ( var CHARCODE = 65500; CHARCODE < 65536; CHARCODE++ ) {
  new TestCase( SECTION,
		"escape(String.fromCharCode("+CHARCODE+"))",
		"%u"+ ToUnicodeString(CHARCODE),
		escape(String.fromCharCode(CHARCODE))  );
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
