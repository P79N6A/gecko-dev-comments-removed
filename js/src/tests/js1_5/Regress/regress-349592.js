




































var gTestfile = 'regress-349592.js';

var BUGNUMBER = 349592;
var summary = 'Do not assert with try/finally inside finally';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  (function() { try { } finally { try { } finally { } } });

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
