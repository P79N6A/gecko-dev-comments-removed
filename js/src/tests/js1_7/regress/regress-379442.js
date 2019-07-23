




































var gTestfile = 'regress-379442.js';

var BUGNUMBER = 379442;
var summary = 'Regression from bug 368224';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  reportCompare(expect, actual, summary);

  (function () { ({ y: [] }) = {} });

  exitFunc ('test');
}
