




































var gTestfile = 'regress-465135.js';

var BUGNUMBER = 465135;
var summary = 'true << true';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = '2,2,2,2,2,';
  actual = '';

  jit(true);

  for (var i=0;i<5;++i) actual += (true << true) + ',';

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
