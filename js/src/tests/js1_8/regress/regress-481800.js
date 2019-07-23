




































var gTestfile = 'regress-481800.js';

var BUGNUMBER = 481800;
var summary = 'TM: Do not assert: (!lhs->isQuad() && !rhs->isQuad()) || (lhs->isQuad() && rhs->isQuad())';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  for each (let x in ['', 0, 0, eval]) { y = x } ( function(){} );

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
