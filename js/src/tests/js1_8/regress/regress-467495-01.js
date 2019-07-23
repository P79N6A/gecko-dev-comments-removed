




































var gTestfile = 'regress-467495-01.js';

var BUGNUMBER = 467495;
var summary = 'Do not crash @ js_Interpret';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  (function() { x = 0; function x() 4; var x; const y = 1; })();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
