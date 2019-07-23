




































var gTestfile = 'regress-351102-07.js';

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
  var obj = { get a() {
      try {
        throw 1;
      } catch (e) {
      }
      return false;
    }};

  try {
    throw obj;
  } catch ({a: a} if a) {
    throw "Unreachable";
  } catch (e) {
    if (e !== obj)
      throw "Unexpected exception: "+uneval(e);
  }
  reportCompare(expect, actual, summary + ': 7');

  exitFunc ('test');
}
