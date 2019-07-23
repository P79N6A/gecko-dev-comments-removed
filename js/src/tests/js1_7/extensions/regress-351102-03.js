




































var gTestfile = 'regress-351102-03.js';

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
        print(y);
      }
    };

  f();
  f();

  reportCompare(expect, actual, summary + ': 3');
  exitFunc ('test');
}
