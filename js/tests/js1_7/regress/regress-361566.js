




































var bug = 361566;
var summary = 'Assertion: !fp->blockChain || OBJ_GET_PARENT(cx, obj) == fp->blockChain';
var actual = '';
var expect = '';


test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  try { let([] = z) 3; } catch(ex) { }
  
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
