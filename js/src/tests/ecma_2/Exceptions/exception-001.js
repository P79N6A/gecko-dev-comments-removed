















var SECTION = "exception-001";
var VERSION = "js1_4";
var TITLE   = "Tests for JavaScript Standard Exceptions:  CallError";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

Call_1();

test();

function Call_1() {
  result = "failed: no exception thrown";
  exception = null;

  try {
    Math();
  } catch ( e ) {
    result = "passed:  threw exception",
      exception = e.toString();
  } finally {
    new TestCase(
      SECTION,
      "Math() [ exception is " + exception +" ]",
      "passed:  threw exception",
      result );
  }
}

