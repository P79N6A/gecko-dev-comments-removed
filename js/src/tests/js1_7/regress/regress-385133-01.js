





var BUGNUMBER = 385133;
var summary = 'Do not crash due to recursion with watch, setter, delete, generator';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  try
  { 
    Object.defineProperty(this, "x", { set: {}.watch, enumerable: true, configurable: true });
    this.watch('x', 'foo'.split);
    delete x;
    function g(){ x = 1; yield; }
    for (i in g()) { }
  }
  catch(ex)
  {
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
