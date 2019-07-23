




































var gTestfile = 'regress-452498-099.js';

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



  try
  {
    eval("(function x(){x.(this)} )();");
  }
  catch(ex)
  {
  }






  try
  {
    (function(){try {x} finally {}; ([x in []] for each (x in x))})();
  }
  catch(ex)
  {
  }






  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
