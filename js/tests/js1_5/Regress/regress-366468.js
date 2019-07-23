





































var bug = 366468;
var summary = 'Set property without setter';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  try
  {
    function o(){} o.prototype = {get foo() {}}; obj = new o(); obj.foo = 2;
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
