





var BUGNUMBER = 359062;
var summary = 'Access generator local variables from nested functions';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = "Generator string";

  var scope = "Global";

  function gen() {
    var scope = "Generator";
    function inner() {
      actual = scope + " " + typeof scope;
    }
    inner();
    yield;
  }

  gen().next();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
