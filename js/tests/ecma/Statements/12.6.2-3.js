





































gTestfile = '12.6.2-3.js';













var SECTION = "12.6.2-3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The for statement";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION, "for statement",  100,     testprogram() );

test();

function testprogram() {
  myVar = 0;

  for ( ; myVar < 100 ; myVar++ ) {
    continue;
  }

  return myVar;
}
