




































var gTestfile = 'regress-344804.js';

var BUGNUMBER = 344804;
var summary = 'Do not crash iterating over window.Packages';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (typeof window != 'undefined')
  {
    for (var p in window.Packages)
      ;
  }
 
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
