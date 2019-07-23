




































var gTestfile = 'regress-355786.js';

var BUGNUMBER = 355786;
var summary = 'Decompilation of for (a[b, this] in []) { }';
var actual = '';
var expect = '';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;

  f = function () { for (a[b, this] in []) { }};
  expect = 'function () { for (a[b, this] in []) { }}';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
