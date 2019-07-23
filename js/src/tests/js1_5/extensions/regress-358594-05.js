




































var gTestfile = 'regress-358594-05.js';


var BUGNUMBER = 358594;
var summary = 'Do not crash on uneval(this).';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  
  f = function () { };
  f.hhhhhhhhh = this; 
  this.m setter = f; 
  uneval(this);
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
