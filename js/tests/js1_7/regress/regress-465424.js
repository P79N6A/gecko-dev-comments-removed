




































var gTestfile = 'regress-465424.js';

var BUGNUMBER = 465424;
var summary = 'TM: issue with post-decrement operator';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = '0,1,2,3,4,';

  jit(true);
  for (let j=0;j<5;++j) { jj=j; print(actual += '' + (jj--) + ',') }
  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
