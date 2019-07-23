





































var gTestfile = 'regress-366468.js';

var BUGNUMBER = 366468;
var summary = 'Set property without setter';
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
    function o(){} o.prototype = {get foo() {}}; obj = new o(); obj.foo = 2;
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
