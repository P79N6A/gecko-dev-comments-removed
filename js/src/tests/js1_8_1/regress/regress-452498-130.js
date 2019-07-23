




































var gTestfile = 'regress-452498-130.js';

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




    let(x=[]) function(){try {x} catch(x) {} }



  try
  {
    eval('for(let [y] = (let (x) (y)) in []) function(){}');
  }
  catch(ex)
  {
  }






  for (var x = 0; x < 3; ++x) { new function(){} }




  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
