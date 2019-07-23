






































var gTestfile = 'regress-58946.js';

var BUGNUMBER = '58946';
var stat =  'Testing a return statement inside a catch statement inside a function';

test();

function test() {
  enterFunc ("test");
  printBugNumber(BUGNUMBER);
  printStatus (stat);

  expect = 'PASS';

  function f()
  {
    try
    {
      throw 'PASS';
    }
    catch(e)
    {
      return e;
    }
  }

  actual = f();

  reportCompare(expect, actual, stat);

  exitFunc ("test");
}
