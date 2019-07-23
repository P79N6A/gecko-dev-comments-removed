




































var gTestfile = 'regress-437288-02.js';

var BUGNUMBER = 437288;
var summary = 'for loop turning into a while loop';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function f() { const x = 1; for (x in null); }

  expect = 'function f() { const x = 1; for (x in null) {} }';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
