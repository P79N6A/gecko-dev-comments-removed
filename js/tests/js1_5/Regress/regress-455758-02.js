




































var gTestfile = 'regress-455758-02.js';

var BUGNUMBER = 455758;
var summary = 'Do not crash: divide by zero';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  (function() { for (var j = 0; j < 5; ++j) { 3 % (-0); } })();

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
