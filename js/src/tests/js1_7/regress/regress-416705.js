




































var gTestfile = 'regress-416705.js';

var BUGNUMBER = 416705;
var summary = 'throw from xml filter crashes';
var actual = 'No Crash';
var expect = 6;



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var g;

  function f()
  {
    try {
      <><a/><b/></>.(let (a=1, b = 2, c = 3)
                     (g = function() { a += b+c; return a; }, xxx));
    } catch (e) {
    }
  }

  f();
  var actual = g();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
