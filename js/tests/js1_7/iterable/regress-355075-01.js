




































var bug = 355075;
var summary = 'Regression tests from bug 354750';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  options('strict');
  options('werror');
  
  function f() {
    this.a = {1: "a", 2: "b"};
    var dummy;
    for (var b in this.a)
      dummy = b;
  }

  f();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
