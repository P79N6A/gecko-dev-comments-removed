




































var gTestfile = 'regress-460501.js';

var BUGNUMBER = 460501;
var summary = 'Decompilation of constant folding with && and valueOf, eval';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f = (function() { if ((1 && w.valueOf())) {} });

  expect = 'function() { if (w.valueOf()) {} }';
  actual = f + '';

  compareSource(expect, actual, summary);

  var f = (function() { if ((1 && w.eval())) {} });

  expect = 'function() { if (w.eval()) {} }';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
