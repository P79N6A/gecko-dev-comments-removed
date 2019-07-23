




































var gTestfile = 'regress-398609.js';

var BUGNUMBER = 398609;
var summary = 'Test regression from bug 398609';
var actual = 'No Error';
var expect = 'No Error';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function f(m) {
    m = m || Math;
    return x();

    function x() {
      return m.sin(0);
    }
  };

  var r = f();
  if (r !== Math.sin(0))
    throw "Unexpected result";

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
