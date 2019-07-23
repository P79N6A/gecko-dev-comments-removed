




































var gTestfile = 'regress-446026-02.js';

var BUGNUMBER = 446026;
var summary = 'brian loves eval(s, o)';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

expect = 'locallocal';

var x = "global";
(function() {
  var x = "local";
  (function() {
    actual = x;
    eval("", {});
    actual += x;
  })();
})();
 
reportCompare(expect, actual, summary);
