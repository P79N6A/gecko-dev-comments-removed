




































var gTestfile = 'regress-465454.js';

var BUGNUMBER = 465454;
var summary = 'TM: do not crash with type-unstable loop';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  for each (let b in [(-1/0), new String(''), new String(''), null, (-1/0),
                      (-1/0), new String(''), new String(''), null]) '' + b;

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
