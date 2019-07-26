





var BUGNUMBER = 407024;
var summary = 'Do not assert pn3->pn_val.isNumber() || pn3->pn_val.isString() || pn3->pn_val.isBoolean()';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

eval("function f(x) { switch (x) { case Array: return 1; }}");
var result = f(Array);
if (result !== 1)
  throw "Unexpected result: "+uneval(result);
 
reportCompare(expect, actual, summary);
