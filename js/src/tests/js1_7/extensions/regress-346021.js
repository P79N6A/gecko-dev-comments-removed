




































var gTestfile = 'regress-346021.js';

var BUGNUMBER = 346021;
var summary = 'Implmenting __iterator__ as generator';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var o = { __iterator__: function () { print(12); yield 42; } };

  expect = 42;
  actual = 0;
 
  for (let i in Iterator(o))
  {
    actual = i;
  }

  reportCompare(expect, actual, summary);

  actual = 0;

  for (let i in o)
  {
    actual = i; 
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
