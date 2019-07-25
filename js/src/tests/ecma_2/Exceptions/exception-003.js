















var SECTION = "exception-003";
var VERSION = "js1_4";
var TITLE   = "Tests for JavaScript Standard Exceptions: TargetError";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

Target_1();

test();

function Target_1() {
  result = "failed: no exception thrown";
  exception = null;

  try {
    string = new String("hi");
    string.toString = Boolean.prototype.toString;
    string.toString();
  } catch ( e ) {
    result = "passed:  threw exception",
      exception = e.toString();
  } finally {
    new TestCase(
      SECTION,
      "string = new String(\"hi\");"+
      "string.toString = Boolean.prototype.toString" +
      "string.toString() [ exception is " + exception +" ]",
      "passed:  threw exception",
      result );
  }
}

