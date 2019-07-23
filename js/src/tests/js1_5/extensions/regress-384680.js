




































var gTestfile = 'regress-384680.js';

var BUGNUMBER = 384680;
var summary = 'Round-trip change in decompilation with paren useless expression';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'function() {}';

  var f = (function() { (3); });
  actual = f + '';
  compareSource(expect, actual, summary + ': f');

  var g = eval('(' + f + ')');
  actual = g + '';
  compareSource(expect, actual, summary + ': g');

  exitFunc ('test');
}
