




































var gTestfile = 'regress-361617.js';

var BUGNUMBER = 361617;
var summary = 'Do not crash with getter, watch and gc';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  (function() { this.x getter= function(){} })();
  this.watch('x', print);
  this.x getter= function(){};
  gc();
  this.unwatch('x');
  x;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
