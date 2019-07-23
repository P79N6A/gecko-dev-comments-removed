





































gTestfile = 'function-001.js';






















var SECTION = "function-001.js";
var VERSION = "JS1_1";
var TITLE   = "functions not separated by semicolons are not errors in version 110 ";
var BUGNUMBER="99232";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

result = "passed";

new TestCase(
  SECTION,
  "eval(\"function f(){}function g(){}\")",
  void 0,
  eval("function f(){}function g(){}") );

test();

