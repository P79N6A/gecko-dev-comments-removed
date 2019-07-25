

















var SECTION = "exception-009";
var VERSION = "JS1_4";
var TITLE   = "Tests for JavaScript Standard Exceptions: SyntaxError";
var BUGNUMBER= "312964";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

try {
  expect = "passed:  no exception thrown";
  result = expect;
  Nested_1();
} catch ( e ) {
  result = "failed: threw " + e;
} finally {
  new TestCase(
    SECTION,
    "nested try",
    expect,
    result );
}


test();

function Nested_1() {
  try {
    try {
    } catch (a) {
    } finally {
    }
  } catch (b) {
  } finally {
  }
}
