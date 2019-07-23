




































var gTestfile = 'regress-452498-076.js';

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



  for (let d = 0; d < 4; ++d) { d; }


  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
