




































var gTestfile = 'regress-466905-02.js';

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
 
  for (var i = 0; i < 5; i++)
    [(i > 3) ? 'a' : 'b'];

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
