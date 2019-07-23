





































gTestfile = '9.7.js';



















var SECTION = "9.7";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Type Conversion:  ToInt16");

















new TestCase( "9.7",   "String.fromCharCode(0).charCodeAt(0)",          ToInt16(0),      String.fromCharCode(0).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(-0).charCodeAt(0)",         ToInt16(0),      String.fromCharCode(-0).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(1).charCodeAt(0)",          ToInt16(1),      String.fromCharCode(1).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(64).charCodeAt(0)",         ToInt16(64),     String.fromCharCode(64).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(126).charCodeAt(0)",        ToInt16(126),    String.fromCharCode(126).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(127).charCodeAt(0)",        ToInt16(127),    String.fromCharCode(127).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(128).charCodeAt(0)",        ToInt16(128),    String.fromCharCode(128).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(130).charCodeAt(0)",        ToInt16(130),    String.fromCharCode(130).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(255).charCodeAt(0)",        ToInt16(255),    String.fromCharCode(255).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(256).charCodeAt(0)",        ToInt16(256),    String.fromCharCode(256).charCodeAt(0) );

new TestCase( "9.7",   "String.fromCharCode(Math.pow(2,16)-1).charCodeAt(0)",   65535,  String.fromCharCode(Math.pow(2,16)-1).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(Math.pow(2,16)).charCodeAt(0)",     0,      String.fromCharCode(Math.pow(2,16)).charCodeAt(0) );

new TestCase( "9.7",   "String.fromCharCode(65535).charCodeAt(0)",     ToInt16(65535),      String.fromCharCode(65535).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(65536).charCodeAt(0)",     ToInt16(65536),      String.fromCharCode(65536).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(65537).charCodeAt(0)",     ToInt16(65537),      String.fromCharCode(65537).charCodeAt(0) );

new TestCase( "9.7",   "String.fromCharCode(131071).charCodeAt(0)",     ToInt16(131071),    String.fromCharCode(131071).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(131072).charCodeAt(0)",     ToInt16(131072),    String.fromCharCode(131072).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(131073).charCodeAt(0)",     ToInt16(131073),    String.fromCharCode(131073).charCodeAt(0) );

new TestCase( "9.7",   "String.fromCharCode('65535').charCodeAt(0)",     65535,             String.fromCharCode("65535").charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode('65536').charCodeAt(0)",     0,                 String.fromCharCode("65536").charCodeAt(0) );

new TestCase( "9.7",   "String.fromCharCode(-1).charCodeAt(0)",         ToInt16(-1),        String.fromCharCode(-1).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(-64).charCodeAt(0)",        ToInt16(-64),       String.fromCharCode(-64).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(-126).charCodeAt(0)",       ToInt16(-126),      String.fromCharCode(-126).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(-127).charCodeAt(0)",       ToInt16(-127),      String.fromCharCode(-127).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(-128).charCodeAt(0)",       ToInt16(-128),      String.fromCharCode(-128).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(-130).charCodeAt(0)",       ToInt16(-130),      String.fromCharCode(-130).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(-255).charCodeAt(0)",       ToInt16(-255),      String.fromCharCode(-255).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(-256).charCodeAt(0)",       ToInt16(-256),      String.fromCharCode(-256).charCodeAt(0) );

new TestCase( "9.7",   "String.fromCharCode(-Math.pow(2,16)-1).charCodeAt(0)",   65535,     String.fromCharCode(-Math.pow(2,16)-1).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(-Math.pow(2,16)).charCodeAt(0)",     0,         String.fromCharCode(-Math.pow(2,16)).charCodeAt(0) );

new TestCase( "9.7",   "String.fromCharCode(-65535).charCodeAt(0)",     ToInt16(-65535),    String.fromCharCode(-65535).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(-65536).charCodeAt(0)",     ToInt16(-65536),    String.fromCharCode(-65536).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(-65537).charCodeAt(0)",     ToInt16(-65537),    String.fromCharCode(-65537).charCodeAt(0) );

new TestCase( "9.7",   "String.fromCharCode(-131071).charCodeAt(0)",    ToInt16(-131071),   String.fromCharCode(-131071).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(-131072).charCodeAt(0)",    ToInt16(-131072),   String.fromCharCode(-131072).charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode(-131073).charCodeAt(0)",    ToInt16(-131073),   String.fromCharCode(-131073).charCodeAt(0) );

new TestCase( "9.7",   "String.fromCharCode('-65535').charCodeAt(0)",   ToInt16(-65535),    String.fromCharCode("-65535").charCodeAt(0) );
new TestCase( "9.7",   "String.fromCharCode('-65536').charCodeAt(0)",   ToInt16(-65536),    String.fromCharCode("-65536").charCodeAt(0) );














test();

function ToInt16( num ) {
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

