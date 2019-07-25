


























var SECTION = "function-001.js";
var VERSION = "JS_12";
var TITLE   = "functions not separated by semicolons are errors in version 120 and higher";
var BUGNUMBER="10278";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "pass";
var exception = "no exception thrown";

try {
  eval("function f(){}function g(){}");
} catch ( e ) {
  result = "fail";
  exception = e.toString();
}

new TestCase(
  SECTION,
  "eval(\"function f(){}function g(){}\") (threw "+exception,
  "pass",
  result );

test();

