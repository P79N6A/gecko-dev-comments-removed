




































var gTestfile = 'regress-465241.js';

var BUGNUMBER = 465241;
var summary = '"0" in [3]';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = true;
  actual = true;

  jit(true);

  for (let j = 0; j < 5; ++j) actual = actual && ("0" in [3]);

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
