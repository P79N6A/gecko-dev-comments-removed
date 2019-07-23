




































var gTestfile = 'regress-462879.js';

var BUGNUMBER = 462879;
var summary = 'Do not assert: UPVAR_FRAME_SKIP(uva->vector[i]) == 1';
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
    (function(c){eval("eval('c.x')")})();
  }
  catch(ex)
  {
  }
 
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
