






var gTestfile = 'regress-424954.js';

var BUGNUMBER = 424954;
var summary = 'Do not crash with [].concat(null)';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  [].concat(null);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
