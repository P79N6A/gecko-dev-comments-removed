





































gTestfile = '15.5.3.2-3.js';




















var SECTION = "15.5.3.2-3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.fromCharCode()";

writeHeaderToLog( SECTION + " "+ TITLE);

for ( CHARCODE = 0; CHARCODE < 256; CHARCODE++ ) {
  new TestCase(   SECTION,
		  "(String.fromCharCode(" + CHARCODE +")).charCodeAt(0)",
		  ToUint16(CHARCODE),
		  (String.fromCharCode(CHARCODE)).charCodeAt(0)
    );
}
for ( CHARCODE = 256; CHARCODE < 65536; CHARCODE+=333 ) {
  new TestCase(   SECTION,
		  "(String.fromCharCode(" + CHARCODE +")).charCodeAt(0)",
		  ToUint16(CHARCODE),
		  (String.fromCharCode(CHARCODE)).charCodeAt(0)
    );
}
for ( CHARCODE = 65535; CHARCODE < 65538; CHARCODE++ ) {
  new TestCase(   SECTION,
		  "(String.fromCharCode(" + CHARCODE +")).charCodeAt(0)",
		  ToUint16(CHARCODE),
		  (String.fromCharCode(CHARCODE)).charCodeAt(0)
    );
}
for ( CHARCODE = Math.pow(2,32)-1; CHARCODE < Math.pow(2,32)+1; CHARCODE++ ) {
  new TestCase(   SECTION,
		  "(String.fromCharCode(" + CHARCODE +")).charCodeAt(0)",
		  ToUint16(CHARCODE),
		  (String.fromCharCode(CHARCODE)).charCodeAt(0)
    );
}
for ( CHARCODE = 0; CHARCODE > -65536; CHARCODE-=3333 ) {
  new TestCase(   SECTION,
		  "(String.fromCharCode(" + CHARCODE +")).charCodeAt(0)",
		  ToUint16(CHARCODE),
		  (String.fromCharCode(CHARCODE)).charCodeAt(0)
    );
}
new TestCase( SECTION, "(String.fromCharCode(65535)).charCodeAt(0)",    65535,  (String.fromCharCode(65535)).charCodeAt(0) );
new TestCase( SECTION, "(String.fromCharCode(65536)).charCodeAt(0)",    0,      (String.fromCharCode(65536)).charCodeAt(0) );
new TestCase( SECTION, "(String.fromCharCode(65537)).charCodeAt(0)",    1,      (String.fromCharCode(65537)).charCodeAt(0) );

test();

function ToUint16( num ) {
  num = Number( num );
  if ( isNaN( num ) || num == 0 || num == Number.POSITIVE_INFINITY || num == Number.NEGATIVE_INFINITY ) {
    return 0;
  }

  var sign = ( num < 0 ) ? -1 : 1;

  num = sign * Math.floor( Math.abs( num ) );
  num = num % Math.pow(2,16);
  num = ( num > -65536 && num < 0) ? 65536 + num : num;
  return num;
}

