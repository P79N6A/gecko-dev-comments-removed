




































var gTestfile = 'regress-452333.js';

var BUGNUMBER = 452333;
var summary = 'Do not crash with JIT: @ js_SkipWhiteSpace';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  (function() { for (var j = 0; j < 5; ++j) { (typeof 3/0); } })();

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
