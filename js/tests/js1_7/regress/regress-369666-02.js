




































var gTestfile = 'regress-369666-02.js';

var BUGNUMBER = 369666;
var summary = 'inner function declaration in let-induced outer ' +
  'function body gets wrong scope.';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  function foo() {
    let x = 42

      function bar() {
      return x;
    }

    return bar;
  }

  print(foo()());

  baz = false;

  function foo2() {
    let x = 42

      function bar() {
      return x;
    }

    function bletch() {
      return x * x;
    }

    try {
      if (baz)
        return bar;
    } finally {
      print('finally', x);
    }
    return bletch;
  }

  print(foo2()());

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
