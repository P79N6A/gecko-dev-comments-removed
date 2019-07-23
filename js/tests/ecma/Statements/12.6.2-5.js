





































gTestfile = '12.6.2-5.js';













var SECTION = "12.6.2-5";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The for statement";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION, "for statement",  99,     testprogram() );

test();

function testprogram() {
  myVar = 0;

  for ( ; myVar < 100 ; myVar++ ) {
    if (myVar == 99)
      break;
  }

  return myVar;
}
