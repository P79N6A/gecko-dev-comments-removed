




































var bug = 345879;
var summary = 'Crash when calling a function from a generator with less arguments than its arity ';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  function gen() {
    yield isNaN();
  }

  f = gen();
  f.next();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
