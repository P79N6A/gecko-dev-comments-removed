




































var gTestfile = 'regress-352092.js';

var BUGNUMBER = 352092;
var summary = 'Do not assert on with() let';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  let(z) { with({}) let y = 3; }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
