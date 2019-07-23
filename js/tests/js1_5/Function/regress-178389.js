







































var gTestfile = 'regress-178389.js';

var BUGNUMBER = 178389;
var summary = 'Function.prototype.toSource should not override Function.prototype.toString';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
function f()
{
  var g = function (){};
}

expect = f.toString();

Function.prototype.toSource = function () { return ''; };

actual = f.toString();

reportCompare(expect, actual, summary);
