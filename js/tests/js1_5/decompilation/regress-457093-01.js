




































var gTestfile = 'regress-457093-01.js';

var BUGNUMBER = 457093;
var summary = 'Do not assert: newtop <= oldtop, decompiling function';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function Test(object)
  {
    for (var i in object)
    {
      try
      {
      }
      catch(E) {}
    }
  }

  var x = Test.toString();
  
  print(x);

  expect = 'function Test ( object ) { for ( var i in object ) { try { } catch ( E ) { } } }';
  actual = Test + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
