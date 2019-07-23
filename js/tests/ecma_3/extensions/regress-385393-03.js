




































var gTestfile = 'regress-385393-03.js';


var BUGNUMBER = 385393;
var summary = 'Regression test for bug 385393';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  f = (function() { new (delete y) });
  eval(uneval(f))

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
