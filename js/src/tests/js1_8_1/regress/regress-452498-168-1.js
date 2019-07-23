




































var gTestfile = 'regress-452498-168-1.js';

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



  (
    new Function("const x = (function () { if (1e+81){} else{x} } )"
      ))();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
