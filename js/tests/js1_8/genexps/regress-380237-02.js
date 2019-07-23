




































var gTestfile = 'regress-380237-02.js';


var BUGNUMBER = 380237;
var summary = 'Decompilation of generator expressions';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function() { g = (d for (d in [0])); g.next(); };
  expect = 'function() { g = (d for (d in [0])); g.next(); }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
