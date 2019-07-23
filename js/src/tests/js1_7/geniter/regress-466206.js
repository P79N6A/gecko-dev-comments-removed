




































var gTestfile = 'regress-466206.js';

var BUGNUMBER = 466206;
var summary = 'Do not crash due to unrooted function variables';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f;

  function g() {
    var x = {};
    f = function () { x.y; };
    if (0) yield;
  }

  try { g().next(); } catch (e) {}
  gc();
  f();
 
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
