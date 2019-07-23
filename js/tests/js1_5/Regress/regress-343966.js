




































var bug = 343966;
var summary = 'ClearScope foo regressed due to bug 343417';
var actual = 'failed';
var expect = 'passed';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  Function["prototype"].inherits=function(a){};
  function foo(){};
  function bar(){};
  foo.inherits(bar);
  actual = "passed";

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
