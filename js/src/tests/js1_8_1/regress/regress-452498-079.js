




































var gTestfile = 'regress-452498-079.js';

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



  x; var x; function x() 0



  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
