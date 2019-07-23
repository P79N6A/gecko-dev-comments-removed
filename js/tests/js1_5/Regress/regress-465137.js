




































var gTestfile = 'regress-465137.js';

var BUGNUMBER = 465137;
var summary = '!NaN is not false';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'falsy,falsy,falsy,falsy,falsy,';
  actual = '';

  jit(true);

  for (var i=0;i<5;++i) actual += (!(NaN) ? "falsy" : "truthy") + ',';

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
