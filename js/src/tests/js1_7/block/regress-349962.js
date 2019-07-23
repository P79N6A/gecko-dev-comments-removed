




































var gTestfile = 'regress-349962.js';

var BUGNUMBER = 349962;
var summary = 'let variable bound to nested function expressions'
  var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  (function() { let z = (function () { function a() { } })(); })()
    reportCompare(expect, actual, summary);

  exitFunc ('test');
}
