




































var gTestfile = 'regress-452498-114.js';

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



  for (var x = 0; x < 3; ++x){ y = function (){} }






  function y([{x: x, y}]){}





  try
  {
    eval("(1.3.__defineGetter__(\"\"));let (y, x) { var z = true, x = 0; }");
  }
  catch(ex)
  {
  }


  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
