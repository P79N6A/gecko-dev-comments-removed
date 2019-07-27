





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

  let x;
  with({x: (x -= 0)}){([]); const x = undefined; }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
