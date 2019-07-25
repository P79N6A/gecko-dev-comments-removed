















var SECTION = "exception-006";
var VERSION = "js1_4";
var TITLE   = "Tests for JavaScript Standard Exceptions: TypeError";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

ToPrimitive_1();

test();







function MyObject() {
  this.toString = void 0;
  this.valueOf = void 0;
}

function ToPrimitive_1() {
  result = "failed: no exception thrown";
  exception = null;

  try {
    result = new MyObject() + new MyObject();
  } catch ( e ) {
    result = "passed:  threw exception",
      exception = e.toString();
  } finally {
    new TestCase(
      SECTION,
      "new MyObject() + new MyObject() [ exception is " + exception +" ]",
      "passed:  threw exception",
      result );
  }
}

