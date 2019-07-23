




































var gTestfile = 'regress-467495-03.js';

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

  function f(x)
  {
    actual = '';
    var g;
    print(actual += typeof g + ',');

    if (x)
      function g(){};

    print(actual += g);
  }

  expect = 'undefined,undefined';
  f(0);

  reportCompare(expect, actual, summary + ': f(0): ');

  expect = 'undefined,function g() {\n}';

  f(1);

  reportCompare(expect, actual, summary + ': f(1): ');

  exitFunc ('test');
}
