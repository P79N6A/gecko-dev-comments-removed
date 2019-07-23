





































var gTestfile = 'regress-299641.js';

var BUGNUMBER = 299641;
var summary = '12.6.4 - LHS for (LHS in expression) execution';
var actual = '';
var expect = 0;

printBugNumber(BUGNUMBER);
printStatus (summary);

function f() {
  var i = 0;
  var a = [{x: 42}];
  for (a[i++].x in [])
    return(a[i-1].x);
  return(i);
}
actual = f();
 
reportCompare(expect, actual, summary);
