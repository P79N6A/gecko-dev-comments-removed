




































var gTestfile = 'regress-352742-02.js';

var BUGNUMBER = 352742;
var summary = 'eval("return") in toString';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 3;
  var j = ({toString: function() { eval("return"); }});
  actual = 2;
  try { "" + j; } catch(e){print(e)}
  actual = 3;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
