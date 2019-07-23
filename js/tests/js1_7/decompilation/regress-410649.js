




































var gTestfile = 'regress-410649.js';

var BUGNUMBER = 410649;
var summary = 'function statement and destructuring parameter name clash';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var h = (function () { return g; function g() { } });
  expect = 'function () { return g; function g() { } }';
  actual = h + '';
  compareSource(expect, actual, '');

  exitFunc ('test');
}
