




































var gTestfile = 'regress-465688.js';

var BUGNUMBER = 465688;
var summary = 'Do not assert: (m != JSVAL_INT) || isInt32(*vp),';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true); 
  for each (let d in [-0x80000000, -0x80000000]) - -d;
  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
