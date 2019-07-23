




































var gTestfile = 'regress-480579.js';

var BUGNUMBER = 480579;
var summary = 'Do not assert: pobj_ == obj2';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = '12';

  a = {x: 1};
  b = {__proto__: a};
  c = {__proto__: b};
  for (i = 0; i < 2; i++) {
    print(actual += c.x);
    b.x = 2;
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
