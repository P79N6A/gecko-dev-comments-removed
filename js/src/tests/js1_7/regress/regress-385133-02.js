





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
    function g() { x = 1; yield; }
    g().next();
  }
  catch(ex)
  {
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
