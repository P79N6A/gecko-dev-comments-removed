





































gTestfile = '15.5.4.5-4.js';











var VERSION = "0697";
startTest();
var SECTION = "15.5.4.5-4";

writeHeaderToLog( SECTION + " String.prototype.charCodeAt(pos)" );

var MAXCHARCODE = Math.pow(2,16);
var item=0, CHARCODE;

for ( CHARCODE=0; CHARCODE <256; CHARCODE++ ) {
  new TestCase( SECTION,
		"(String.fromCharCode("+CHARCODE+")).charCodeAt(0)",
		CHARCODE,
		(String.fromCharCode(CHARCODE)).charCodeAt(0) );
}
for ( CHARCODE=256; CHARCODE < 65536; CHARCODE+=999 ) {
  new TestCase( SECTION,
		"(String.fromCharCode("+CHARCODE+")).charCodeAt(0)",
		CHARCODE,
		(String.fromCharCode(CHARCODE)).charCodeAt(0) );
}

new TestCase( SECTION, "(String.fromCharCode(65535)).charCodeAt(0)", 65535,     (String.fromCharCode(65535)).charCodeAt(0) );

test();
