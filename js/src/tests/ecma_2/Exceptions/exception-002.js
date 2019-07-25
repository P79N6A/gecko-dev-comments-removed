















var SECTION = "exception-002";
var VERSION = "js1_4";
var TITLE   = "Tests for JavaScript Standard Exceptions: ConstructError";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

Construct_1();

test();

function Construct_1() {
  result = "failed: no exception thrown";
  exception = null;

  try {
    result = new Math();
  } catch ( e ) {
    result = "passed:  threw exception",
      exception = e.toString();
  } finally {
    new TestCase(
      SECTION,
      "new Math() [ exception is " + exception +" ]",
      "passed:  threw exception",
      result );
  }
}

