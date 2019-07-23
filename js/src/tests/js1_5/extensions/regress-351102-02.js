




































var gTestfile = 'regress-351102-02.js';

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
  f = function ()
    {
      var a = null;
      try {  
        a();
      } catch (e) {
      }
      return false;
    };

  try {  
    throw 1;
  } catch (e if f()) {
  } catch (e if e == 1) {
    print("GOOD");
  } catch (e) {
    print("BAD: "+e);
  }

  reportCompare(expect, actual, summary + ': 2');
  exitFunc ('test');
}
