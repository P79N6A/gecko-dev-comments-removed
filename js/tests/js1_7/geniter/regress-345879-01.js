




































var gTestfile = 'regress-345879-01.js';

var BUGNUMBER = 345879;
var summary = 'Crash when calling a function from a generator with less arguments than its arity ';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function gen() {
    yield isNaN();
  }

  f = gen();
  f.next();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
