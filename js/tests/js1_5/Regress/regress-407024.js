




































var gTestfile = 'regress-407024.js';

var BUGNUMBER = 407024;
var summary = 'Do not assert JSVAL_IS_NUMBER(pn3->pn_val) || JSVAL_IS_STRING(pn3->pn_val) || JSVAL_IS_BOOLEAN(pn3->pn_val)';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

eval("function f(x) { switch (x) { case Array: return 1; }}");
var result = f(Array);
if (result !== 1)
  throw "Unexpected result: "+uneval(result);
 
reportCompare(expect, actual, summary);
