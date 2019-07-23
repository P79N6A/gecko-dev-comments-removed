




































var gTestfile = 'regress-471660.js';

var BUGNUMBER = 471660;
var summary = 'TM: Do not assert: !(fp->flags & JSFRAME_POP_BLOCKS)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  y = <x/>;

  for (var w = 0; w < 5; ++w) {

    let (y) { do break ; while (true); }
    for each (let x in [{}, function(){}]) {y}

  }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
