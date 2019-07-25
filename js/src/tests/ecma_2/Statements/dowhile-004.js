
















var SECTION = "dowhile-004";
var VERSION = "ECMA_2";
var TITLE   = "do...while with a labeled continue statement";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

DoWhile( 0, 1 );
DoWhile( 1, 1 );
DoWhile( -1, 1 );
DoWhile( 5, 5 );

test();

function DoWhile( limit, expect ) {
  i = 0;
  result1 = "pass";
  result2 = "failed: broke out of labeled statement unexpectedly";

foo: {
    do {
      i++;
      if ( ! (i < limit) ) {
	break;
	result1 = "fail: evaluated statement after a labeled break";
      }
    } while ( true );

    result2 = "pass";
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
