




































var gTestfile = 'regress-358594-03.js';


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
  f.__proto__ = this; 
  Object.defineProperty(this, "m", { set: f, enumerable: true, configurable: true });
  uneval(this);
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
