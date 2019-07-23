




































var gTestfile = 'regress-452498-138.js';

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





  ((function x(){ yield (x = undefined) } ) for (y in /x/));



  try
  {
    for(let x in ( x for (y in x) for each (x in []) )) y;
  }
  catch(ex)
  {
  }



  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
