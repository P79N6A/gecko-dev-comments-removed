




































var bug = 373827;
var summary = 'Assertion: OBJ_GET_CLASS(cx, obj)->flags & JSCLASS_HAS_PRIVATE';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  let ([] = [{x: function(){}}]) { };

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
