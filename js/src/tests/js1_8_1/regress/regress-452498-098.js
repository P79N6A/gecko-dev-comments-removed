





































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



  uneval(function(){(Number(0) for each (NaN in []) for each (x4 in this))});


  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
