




































var gTestfile = 'regress-355992.js';

var BUGNUMBER = 355992;
var summary = 'Non-function setter';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function() { return { x setter: 3 }; }
  expect = 'function() { return { x setter: 3 }; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
