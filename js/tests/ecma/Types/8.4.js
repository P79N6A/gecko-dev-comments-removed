





































gTestfile = '8.4.js';


















var SECTION = "8.4";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The String type";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "var s = ''; s.length",
	      0,
	      eval("var s = ''; s.length") );

new TestCase( SECTION,
	      "var s = ''; s.charAt(0)",
	      "",
	      eval("var s = ''; s.charAt(0)") );


for ( var i = 0x0041, TEST_STRING = "", EXPECT_STRING = ""; i < 0x007B; i++ ) {
  TEST_STRING += ("\\u"+ DecimalToHexString( i ) );
  EXPECT_STRING += String.fromCharCode(i);
}

new TestCase( SECTION,
	      "var s = '" + TEST_STRING+ "'; s",
	      EXPECT_STRING,
	      eval("var s = '" + TEST_STRING+ "'; s") );

new TestCase( SECTION,
	      "var s = '" + TEST_STRING+ "'; s.length",
	      0x007B-0x0041,
	      eval("var s = '" + TEST_STRING+ "'; s.length") );

test();

function DecimalToHexString( n ) {
  n = Number( n );
  var h = "";

  for ( var i = 3; i >= 0; i-- ) {
    if ( n >= Math.pow(16, i) ){
      var t = Math.floor( n  / Math.pow(16, i));
      n -= t * Math.pow(16, i);
      if ( t >= 10 ) {
	if ( t == 10 ) {
	  h += "A";
	}
	if ( t == 11 ) {
	  h += "B";
	}
	if ( t == 12 ) {
	  h += "C";
	}
	if ( t == 13 ) {
	  h += "D";
	}
	if ( t == 14 ) {
	  h += "E";
	}
	if ( t == 15 ) {
	  h += "F";
	}
      } else {
	h += String( t );
      }
    } else {
      h += "0";
    }
  }

  return h;
}

