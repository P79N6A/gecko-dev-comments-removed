















var SECTION = "exception-007";
var VERSION = "js1_4";
var TITLE   = "Tests for JavaScript Standard Exceptions:  TypeError";
var BUGNUMBER="318250";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

DefaultValue_1();

test();







function MyObject() {
  this.toString = void 0;
  this.valueOf = new Object();
}

function DefaultValue_1() {
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

