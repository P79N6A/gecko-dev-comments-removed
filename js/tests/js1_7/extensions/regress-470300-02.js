




































var gTestfile = 'regress-470300-02.js';

var BUGNUMBER = 470300;
var summary = 'TM: Do not assert: !fp->blockChain || OBJ_GET_PARENT(cx, obj) == fp->blockChain';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  for (let a = 0; a < 7; ++a) { let e = 8; if (a > 3) { let x; } }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
