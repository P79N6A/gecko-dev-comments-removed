




































var bug = 375695;
var summary = 'Assertion: !fp->blockChain || OBJ_GET_PARENT(cx, obj) == fp->blockChain';
var actual = '';
var expect = '';


test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  try { try { throw 1 } catch([] if false) { } } catch(ex) {}
  
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
