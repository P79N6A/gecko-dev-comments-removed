




































var gTestfile = 'regress-451483.js';

var BUGNUMBER = 451483;
var summary = '[].splice.call(0) == []';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = true;
  var result = [].splice.call(0);
  print('[].splice.call(0) = ' + result);
  actual = result instanceof Array && result.length == 0;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
