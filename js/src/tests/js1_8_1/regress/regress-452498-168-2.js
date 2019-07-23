




































var gTestfile = 'regress-452498-168-2.js';

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



  for (let x; __defineSetter__; (<{x}></{x}> for (x in x))) {}

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
