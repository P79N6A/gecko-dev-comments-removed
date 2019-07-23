




































var gTestfile = 'regress-375976.js';


var BUGNUMBER = 375976;
var summary = 'Do not crash with postincrement custom property';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  this.__defineSetter__('x', gc);
  this.__defineGetter__('x', Math.sin);
  x = x++;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
