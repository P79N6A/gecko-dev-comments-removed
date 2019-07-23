




































var gTestfile = 'regress-464096.js';

var BUGNUMBER = 464096;
var summary = 'TM: Do not assert: tm->recoveryDoublePoolPtr > tm->recoveryDoublePool';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  for (let f in [1,1]);
  Object.prototype.__defineGetter__('x', function() gc());
  (function() { for each (let j in [1,1,1,1,1]) { var y = .2; } })();

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
