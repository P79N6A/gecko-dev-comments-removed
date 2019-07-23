




































var gTestfile = 'regress-458076.js';

var BUGNUMBER = 458076;
var summary = 'Do not assert with JIT: !lhs->isQuad() && !rhs->isQuad()';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  for (let j = 0; j < 3; ++j) { true == 0; }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
