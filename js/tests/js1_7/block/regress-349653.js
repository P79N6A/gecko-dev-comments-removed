




































var bug = 349653;
var summary = 'Assertion failure: OBJ_GET_CLASS(cx, obj) == &js_ArrayClass';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  void ({y: true ? [1 for (x in [2])] : 3 })
  reportCompare(expect, actual, summary);

  let (a) true ? [2 for each (z in function(id) { return id })] : 3;
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
