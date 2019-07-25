















var SECTION = "try-003";
var VERSION = "ECMA_2";
var TITLE   = "The try statement";
var BUGNUMBER="http://scopus.mcom.com/bugsplat/show_bug.cgi?id=313585";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);



TrySomething( "x = \"hi\"", false );
TrySomething( "throw \"boo\"", true );
TrySomething( "throw 3", true );

test();







function TrySomething( expression, throwing ) {
  innerFinally = "FAIL: DID NOT HIT INNER FINALLY BLOCK";
  if (throwing) {
    outerCatch = "FAILED: NO EXCEPTION CAUGHT";
  } else {
    outerCatch = "PASS";
  }
  outerFinally = "FAIL: DID NOT HIT OUTER FINALLY BLOCK";

  try {
    try {
      eval( expression );
    } finally {
      innerFinally = "PASS";
    }
  } catch ( e  ) {
    if (throwing) {
      outerCatch = "PASS";
    } else {
      outerCatch = "FAIL: HIT OUTER CATCH BLOCK";
    }
  } finally {
    outerFinally = "PASS";
  }


  new TestCase(
    SECTION,
    "eval( " + expression +" )",
    "PASS",
    innerFinally );
  new TestCase(
    SECTION,
    "eval( " + expression +" )",
    "PASS",
    outerCatch );
  new TestCase(
    SECTION,
    "eval( " + expression +" )",
    "PASS",
    outerFinally );


}
