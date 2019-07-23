




































var gTestfile = 'regress-465133.js';

var BUGNUMBER = 465133;
var summary = '{} < {}';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'false,false,false,false,false,';
  actual = '';

  jit(true);

  for (var i=0;i<5;++i) actual += ({} < {}) + ',';

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
