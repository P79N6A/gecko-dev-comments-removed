




































var gTestfile = 'regress-464978.js';

var BUGNUMBER = 464978;
var summary = 'Do not hang with [] + null';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  for (let j = 0; j < 2; ++j) { [] + null; }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
