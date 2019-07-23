




































var gTestfile = 'regress-465308.js';

var BUGNUMBER = 465308;
var summary = '((0x60000009) * 0x60000009))';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = '-1073741824,-1073741824,-1073741824,-1073741824,-1073741824,';

  jit(true);
  for (let j=0;j<5;++j) 
    print(actual += "" + (0 | ((0x60000009) * 0x60000009)) + ',');
  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
