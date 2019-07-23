




































var gTestfile = 'regress-390597.js';

var BUGNUMBER = 390597;
var summary = 'watch point + eval-as-setter allows access to dead JSStackFrame';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function exploit() {
    try
    {
    var obj = this, args = null;
    obj.__defineSetter__("evil", eval);
    obj.watch("evil", function() { return "args = arguments;"; });
    obj.evil = null;
    eval("print(args[0]);");
    }
    catch(ex)
    {
      print('Caught ' + ex);
    }
  }
  exploit();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
