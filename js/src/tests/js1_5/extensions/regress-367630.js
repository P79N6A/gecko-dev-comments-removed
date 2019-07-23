




































var gTestfile = 'regress-367630.js';

var BUGNUMBER = 367630;
var summary = 'Do not crash in js_PCToLineNumber with invalid sharp expression';
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
    uneval(#1={a:#1#}); (function() { return #1# })();
  }
  catch(ex)
  {
    print(ex);
  }
  reportCompare(expect, actual, summary);

  try
  {
    w = {a:#1=function(){return #1#}}; w.a();
  }
  catch(ex)
  {
    print(ex);
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
