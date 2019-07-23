




































var gTestfile = 'regress-466905-01.js';

var BUGNUMBER = 466905;
var summary = 'Do not assert: v_ins->isCall() && v_ins->callInfo() == &js_FastNewArray_ci';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function f(a) { for each (let c in a) [(c > 5) ? 'A' : 'B']; }
  f([true, 8]);
  f([2]);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
