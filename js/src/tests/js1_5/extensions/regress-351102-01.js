




































var gTestfile = 'regress-351102-01.js';

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

  f = function () {
    try {
      throw new Error('bad');
    } catch (e if (e = null, gc(), false)) {
    } catch (e) {
      
    }
  };

  f();

  reportCompare(expect, actual, summary + ': 1');

  exitFunc ('test');
}
