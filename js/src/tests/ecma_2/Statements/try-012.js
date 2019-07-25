
















var SECTION = "try-012";
var VERSION = "ECMA_2";
var TITLE   = "The try statement";
var BUGNUMBER="336872";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);



TrySomething( "x = \"hi\"", true );
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
      throw 0;
    } finally {
      innerFinally = "PASS";
      eval( expression );
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
    "eval( " + expression +" ): evaluated inner finally block",
    "PASS",
    innerFinally );
  new TestCase(
    SECTION,
    "eval( " + expression +" ): evaluated outer catch block ",
    "PASS",
    outerCatch );
  new TestCase(
    SECTION,
    "eval( " + expression +" ):  evaluated outer finally block",
    "PASS",
    outerFinally );
}
