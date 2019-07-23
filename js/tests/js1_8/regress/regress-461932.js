




































var gTestfile = 'regress-461932.js';

var BUGNUMBER = 461932;
var summary = 'TM: Do not crash in nanojit::LIns::isop';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  function gen() { for (var j = 0; j < 4; ++j) { NaN; yield 3; } }
  for (let i in gen()) { }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
