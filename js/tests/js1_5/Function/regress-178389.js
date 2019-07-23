







































var bug = 178389;
var summary = 'Function.prototype.toSource should not override Function.prototype.toString';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);
  
function f()
{
  var g = function (){};
}

expect = f.toString();

Function.prototype.toSource = function () { return ''; };

actual = f.toString();

reportCompare(expect, actual, summary);
