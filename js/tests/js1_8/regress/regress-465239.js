




































var gTestfile = 'regress-465239.js';

var BUGNUMBER = 465239;
var summary = '"1e+81" ^  3';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = '3,3,3,3,3,';
  actual = '';

  jit(true);

  for (let j = 0; j < 5; ++j) actual += ("1e+81" ^  3) + ',';

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
