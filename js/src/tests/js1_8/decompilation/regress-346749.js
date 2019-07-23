




































var gTestfile = 'regress-346749.js';

var BUGNUMBER = 346749;
var summary = 'let declarations at function level should turn into var declarations';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'function () { var k; }';
  var f = (function () { let k; });
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
