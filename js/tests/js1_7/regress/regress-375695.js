




































var gTestfile = 'regress-375695.js';

var BUGNUMBER = 375695;
var summary = 'Do not assert: !fp->blockChain || OBJ_GET_PARENT(cx, obj) == fp->blockChain';
var actual = '';
var expect = '';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  try { try { throw 1 } catch([] if false) { } } catch(ex) {}
 
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
