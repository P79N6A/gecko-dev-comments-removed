




































var gTestfile = 'regress-379245.js';

var BUGNUMBER = 379245;
var summary = 'inline calls';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var fThis;

  function f()
  {
    fThis = this;
    return ({x: f}).x;
  }

  f()();

  if (this !== fThis)
    throw "bad this";

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
