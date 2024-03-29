





var BUGNUMBER = 417893;
var summary = 'Fast natives must use JS_THIS/JS_THIS_OBJECT';
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
    (function() { var s = function(){}.prototype.toSource; s(); })();
  }
  catch (e)
  {
    assertEq(e instanceof TypeError, true,
             "No TypeError for Object.prototype.toSource");
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
