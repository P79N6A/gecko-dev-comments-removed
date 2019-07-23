




































var gTestfile = 'regress-383721.js';

var BUGNUMBER = 383721;
var summary = 'decompiling Tabs';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f = function () {return "\t"};
  expect = 'function () {return "\\t";}';
  actual = f + '';
  compareSource(expect, actual, summary + ': toString');

  expect = '(' + expect + ')';
  actual = uneval(f);
  compareSource(expect, actual, summary + ': uneval');

  exitFunc ('test');
}
