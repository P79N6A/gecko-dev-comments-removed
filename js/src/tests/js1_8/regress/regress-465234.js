




































var gTestfile = 'regress-465234.js';

var BUGNUMBER = 465234;
var summary = '"" <= null';
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

  for (let j = 0; j < 5; ++j) actual = actual && ("" <= null);

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
