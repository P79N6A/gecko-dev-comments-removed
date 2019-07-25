



















var SECTION = "dowhile-005";
var VERSION = "ECMA_2";
var TITLE   = "do...while with a labeled continue statement";
var BUGNUMBER = "316293";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

NestedLabel();


test();

function NestedLabel() {
  i = 0;
  result1 = "pass";
  result2 = "fail: did not hit code after inner loop";
  result3 = "pass";

outer: {
    do {
    inner: {

	break inner;
	result1 = "fail: did break out of inner label";
      }
      result2 = "pass";
      break outer;
      print(i);
    } while ( i++ < 100 );

  }

  result3 = "fail: did not break out of outer label";

  new TestCase(
    SECTION,
    "number of loop iterations",
    0,
    i );

  new TestCase(
    SECTION,
    "break out of inner loop",
    "pass",
    result1 );

  new TestCase(
    SECTION,
    "break out of outer loop",
    "pass",
    result2 );
}
