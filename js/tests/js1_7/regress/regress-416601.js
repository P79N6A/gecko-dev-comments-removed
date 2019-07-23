




































var gTestfile = 'regress-416601.js';

var BUGNUMBER = 416601;
var summary = 'Property cache can be left disabled after exit from a generator or trap handler';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function f()
  {
    with (Math) {
      yield 1;
    }
  }

  var iter = f();
  iter.next();
  iter.close();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
