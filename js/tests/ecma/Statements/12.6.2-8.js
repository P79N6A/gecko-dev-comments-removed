





































gTestfile = '12.6.2-8.js';













var SECTION = "12.6.2-8";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The for statement";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION, "for statement",  256,     testprogram() );

test();

function testprogram() {
  var myVar;

  for ( myVar=2;  myVar < 256; myVar *= myVar ) {
  }

  return myVar;
}
