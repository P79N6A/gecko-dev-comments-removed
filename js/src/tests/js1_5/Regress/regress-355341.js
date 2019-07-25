





































var BUGNUMBER = 355341;
var summary = 'Do not crash with watch and setter';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  Object.defineProperty(this, "x", { set: Function, enumerable: true, configurable: true });
  this.watch('x', function () { }); x = 3;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
