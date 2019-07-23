




































var gTestfile = 'regress-452346.js';

var BUGNUMBER = 452346;
var summary = 'Do not crash: @ Balloc';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  for (var j=0;j<2;++j) (0.1).toPrecision(30);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
