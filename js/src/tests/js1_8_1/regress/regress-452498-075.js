




































var gTestfile = 'regress-452498-075.js';

var BUGNUMBER = 452498;
var summary = 'TM: upvar2 regression tests';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);



  (function p(){ p = 3; });
  (function p(){ p = 3; return p; })()


    reportCompare(expect, actual, summary);

  exitFunc ('test');
}
