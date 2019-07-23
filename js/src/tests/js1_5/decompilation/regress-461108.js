




































var gTestfile = 'regress-461108.js';

var BUGNUMBER = 461108;
var summary = 'Decompilation of for (i = 0; a = as[i]; ++i)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f = (function() {for (i = 0; attr = attrs[i]; ++i) {} });

  expect = 'function() {for (i = 0; attr = attrs[i]; ++i) {} }';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
