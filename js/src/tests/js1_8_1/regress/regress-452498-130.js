





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





  ((function x()x in []) for (y in []))







  for (var x = 0; x < 3; ++x) { new function(){} }




  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
