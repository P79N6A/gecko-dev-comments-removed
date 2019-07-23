




































var gTestfile = 'regress-465272.js';

var BUGNUMBER = 465272;
var summary = 'subtraction';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  expect = '3,3,3,3,3,';

  for (j=0;j<5;++j) print(actual += "" + ((5) - 2) + ',');

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
