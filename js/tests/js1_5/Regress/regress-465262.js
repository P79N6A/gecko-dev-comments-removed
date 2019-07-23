




































var gTestfile = 'regress-465262.js';

var BUGNUMBER = 465262;
var summary = 'truthiness of (3 > null)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  expect = 'true,true,true,true,true,';

  for(j=0;j<5;++j) print(actual += "" + (3 > null) + ',')

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
