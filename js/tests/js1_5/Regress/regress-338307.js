




































var gTestfile = 'regress-338307.js';

var BUGNUMBER = 338307;
var summary = 'for (i in arguments) causes type error (JS_1_7_ALPHA_BRANCH)';
var actual = '';
var expect = 'No Error';

printBugNumber(BUGNUMBER);
printStatus (summary);

function f() {
  for (var i in arguments);
}

try
{
  f();
  actual = 'No Error';
}
catch(ex)
{
  actual = ex + '';
}
 
reportCompare(expect, actual, summary);
