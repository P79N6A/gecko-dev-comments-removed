




































var gTestfile = 'regress-452498-038.js';

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



  [0 for (a in [])];

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
