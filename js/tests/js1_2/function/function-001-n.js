





































gTestfile = 'function-001-n.js';






















var SECTION = "function-001.js";
var VERSION = "JS1_1";
var TITLE   = "functions not separated by semicolons are errors in version 120 and higher";
var BUGNUMBER="99232";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase(
  SECTION,
  "eval(\"function f(){}function g(){}\")",
  "error",
  eval("function f(){}function g(){}") );

test();

