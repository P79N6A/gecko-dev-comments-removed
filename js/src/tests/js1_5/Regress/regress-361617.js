





































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
 
  (function() {
    Object.defineProperty(this, "x", { get: function(){}, enumerable: true, configurable: true });
  })();
  this.watch('x', print);
  Object.defineProperty(this, "x", { get: function(){}, enumerable: true, configurable: true });
  gc();
  this.unwatch('x');
  x;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
