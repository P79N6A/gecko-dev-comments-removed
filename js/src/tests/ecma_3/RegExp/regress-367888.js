




































var gTestfile = 'regress-367888.js';

var BUGNUMBER = 367888;
var summary = 'RegExp /(|)??x/g.exec("y") barfs';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = null;
  actual = /(|)??x/g.exec("y");

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
