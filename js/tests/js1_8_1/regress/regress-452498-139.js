




































var gTestfile = 'regress-452498-139.js';

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
    (function(){var x = x (x() for each (x in []))})();
  }
  catch(ex)
  {
  }




  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
