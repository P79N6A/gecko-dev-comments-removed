















var SECTION = "dowhile-002";
var VERSION = "ECMA_2";
var TITLE   = "do...while with a labeled continue statement";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

LabeledContinue( 0, 1 );
LabeledContinue( 1, 1 );
LabeledContinue( -1, 1 );
LabeledContinue( 5, 5 );

test();

function LabeledContinue( limit, expect ) {
  i = 0;
woohoo:
  do {
    i++;
    continue woohoo;
  } while ( i < limit );

  new TestCase(
    SECTION,
    "do while ( " + i +" < " + limit +" )",
    expect,
    i );
}
