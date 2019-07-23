




































var gTestfile = 'regress-467495-05.js';

var BUGNUMBER = 467495;
var summary = 'TCF_FUN_CLOSURE_VS_VAR is necessary';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'function x() {\n}';

  function g(x) { if (1) function x() {} return x; }
  print(actual = g(1) + '');

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
