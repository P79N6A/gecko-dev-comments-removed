













var SECTION = "statement-005";
var VERSION = "JS1_4";
var TITLE   = "The for..in statement";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  var o = new MyObject();
  result = 0;

  eval("for (1 in o) {\n"
       + "result += this[p];"
       + "}\n");
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "bad left-hand side expression" +
  " (threw " + exception +")",
  expect,
  result );

test();

function MyObject() {
  this.value = 2;
  this[0] = 4;
  return this;
}
