




















































var gTestfile = 'regress-72773.js';
var BUGNUMBER = 72773;
var summary = "Regression test: we shouldn't crash on this code";
var status = '';
var actual = '';
var expect = '';
var sToEval = '';





sToEval += 'function Cow(name){this.name = name;}'
sToEval += 'function Calf(str){this.name = str;}'
sToEval += 'Calf.prototype = Cow;'
sToEval += 'new Calf().toString();'

status = 'Trying to catch an expected error';
try
{
  eval(sToEval);
}
catch(e)
{
  actual = getJSClass(e);
  expect = 'Error';
}



test();



function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  reportCompare(expect, actual, status);

  exitFunc ('test');
}
