





































gTestfile = '12.6.2-6.js';













var SECTION = "12.6.2-6";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The for statement";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( "12.6.2-6", "for statement",  256,     testprogram() );

test();

function testprogram() {
  var myVar;

  for ( myVar=2; ; myVar *= myVar ) {

    if (myVar > 100)
      break;
    continue;
  }

  return myVar;
}
