




































var gTestfile = 'regress-344959.js';

var BUGNUMBER = 344959;
var summary = 'Functions should not lose scope chain after exception';
var actual = '';
var expect = 'with';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var x = "global"

    with ({x:"with"})
    actual = (function() { try {} catch(exc) {}; return x }());

  reportCompare(expect, actual, summary + ': 1');

  with ({x:"with"})
    actual = (function() { try { throw 1} catch(exc) {}; return x }());

  reportCompare(expect, actual, summary + ': 2');

  exitFunc ('test');
}
