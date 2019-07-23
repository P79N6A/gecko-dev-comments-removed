




































var gTestfile = 'regress-452498-224.js';

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


  ((#0={}) for(x in null));

  reportCompare(expect, actual, summary + ': 2');

  exitFunc ('test');
}
