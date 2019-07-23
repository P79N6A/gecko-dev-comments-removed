




































var gTestfile = 'regress-429248.js';

var BUGNUMBER = 429248;
var summary = 'Do not assert: 0';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function c() { do{}while(0) }

  if (typeof trap == 'function')
  {
    trap(c, 0, "");
  }
  c + '';

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
