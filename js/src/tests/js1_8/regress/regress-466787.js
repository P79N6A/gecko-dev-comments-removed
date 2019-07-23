




































var gTestfile = 'regress-466787.js';

var BUGNUMBER = 466787;
var summary = 'TM: new Number() should stay a number during recording';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = '4444';

  jit(true);

  for (let j = 0; j < 4; ++j) { for each (let one in [new Number(1)]) {
        print(actual += '' + (3 + one)); } }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
