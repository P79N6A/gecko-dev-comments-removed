





var BUGNUMBER = 351102;
var summary = 'try/catch-guard/finally GC issues';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;
  f = function()
    {
      try
      {
        d.d.d;
      }
      catch([] if gc())
      {
      }
      catch(y)
      {
      }
    };

  f();
  f();

  reportCompare(expect, actual, summary + ': 3');
  exitFunc ('test');
}
