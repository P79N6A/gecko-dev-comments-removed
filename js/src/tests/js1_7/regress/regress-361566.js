




































var gTestfile = 'regress-361566.js';

var BUGNUMBER = 361566;
var summary = 'Do not assert: !fp->blockChain || OBJ_GET_PARENT(cx, obj) == fp->blockChain';
var actual = '';
var expect = '';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  try { let([] = z) 3; } catch(ex) { }
 
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
