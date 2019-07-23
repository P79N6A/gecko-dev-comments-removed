




































var gTestfile = 'regress-470758-02.js';

var BUGNUMBER = 470758;
var summary = 'Promote evald initializer into upvar';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 5;

  (function(){var x;eval("for (x = 0; x < 5; x++);");print(actual = x);})();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
