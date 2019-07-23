





































gTestfile = '12.6.2-1.js';














var SECTION = "12.6.2-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The for statement";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( "12.6.2-1", "for statement",  99,     testprogram() );

test();


function testprogram() {
  myVar = 0;

  for ( ; ; ) {
    if ( ++myVar == 99 )
      break;
  }

  return myVar;
}
