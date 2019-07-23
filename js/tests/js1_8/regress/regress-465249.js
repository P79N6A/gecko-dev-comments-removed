




































var gTestfile = 'regress-465249.js';

var BUGNUMBER = 465249;
var summary = 'Do not assert: (m != JSVAL_INT) || isInt32(*vp)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  eval("for (let j = 0; j < 5; ++j) { (0x50505050) + (0x50505050); }");

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
