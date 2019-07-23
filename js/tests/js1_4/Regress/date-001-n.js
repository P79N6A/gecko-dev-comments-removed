





































gTestfile = 'date-001-n.js';










var SECTION = "date-001-n.js";
var VERSION = "JS1_4";
var TITLE   = "Regression test case for 299903";
var BUGNUMBER="299903";

startTest();

writeHeaderToLog( SECTION + " "+ TITLE);

function MyDate() {
  this.foo = "bar";
}
MyDate.prototype = new Date();

DESCRIPTION =
  "function MyDate() { this.foo = \"bar\"; }; MyDate.prototype = new Date(); new MyDate().toString()";
EXPECTED = "error";

new TestCase(
  SECTION,
  "function MyDate() { this.foo = \"bar\"; }; "+
  "MyDate.prototype = new Date(); " +
  "new MyDate().toString()",
  "error",
  new MyDate().toString() );

test();
