





































var gTestfile = 'regress-451884.js';

var BUGNUMBER = 451884;
var summary = 'Do not crash [@ QuoteString]';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  try
  {
    (function(k){eval("k.y")})();
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
