




































var gTestfile = 'regress-468711.js';

var BUGNUMBER = 468711;
var summary = 'TM: Do not assert: !JS_ON_TRACE(cx)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  (5).toString(); for each (let b in [3, this]) { b.toString(); }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
