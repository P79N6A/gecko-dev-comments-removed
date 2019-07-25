

















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
  result1 = "pass";
  result2 = "pass";

woohoo: {
    do {
      i++;
      if ( ! (i < limit) ) {
	break woohoo;
	result1 = "fail: evaluated statement after a labeled break";
      }
    } while ( true );

    result2 = "failed:  broke out of loop, but not out of labeled block";
  }

  new TestCase(
    SECTION,
    "do while ( " + i +" < " + limit +" )",
    expect,
    i );

  new TestCase(
    SECTION,
    "breaking out of a do... while loop",
    "pass",
    result1 );


  new TestCase(
    SECTION,
    "breaking out of a labeled do...while loop",
    "pass",
    result2 );
}
