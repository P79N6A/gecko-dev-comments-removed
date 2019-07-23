




































var gTestfile = 'regress-470176.js';

var BUGNUMBER = 470176;
var summary = 'let-fun should not be able to modify constants';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  const e = 8; 

  expect = e;

  jit (true);

  let (f = function() { for (var h=0;h<6;++h) ++e; }) { f(); }

  actual = e;

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
