




































var gTestfile = 'regress-356250.js';

var BUGNUMBER = 356250;
var summary = 'Do not assert: !fp->fun || !(fp->fun->flags & JSFUN_HEAVYWEIGHT) || fp->callobj';
var actual = 'No Crash';
var expect = 'No Crash';

(function() { eval("(function() { })"); })();
reportCompare(expect, actual, summary + ': nested 0');


test1();
test2();


function test1()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  (function() { eval("(function() { })"); })();

  reportCompare(expect, actual, summary + ': nested 1');

  exitFunc ('test');
}

function test2()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  (function () {(function() { eval("(function() { })"); })();})();

  reportCompare(expect, actual, summary + ': nested 2');

  exitFunc ('test');
}
