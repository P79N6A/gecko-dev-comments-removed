





































var gTestfile = 'regress-165201.js';

var BUGNUMBER = 165201;
var summary = '';
var actual = '';
var expect = '';


summary = 'RegExp.prototype.toSource should not affect RegExp.prototype.toString';

printBugNumber(BUGNUMBER);
printStatus (summary);






function f()
{
  return /abc/;
}

RegExp.prototype.toSource = function() { return 'Hi there'; };

expect = -1;
actual = f.toString().indexOf('Hi there');

reportCompare(expect, actual, summary);





summary = 'Array.prototype.toSource should not affect Array.prototype.toString';
printBugNumber(BUGNUMBER);
printStatus (summary);

function g()
{
  return [1,2,3];
}

Array.prototype.toSource = function() { return 'Hi there'; }

  expect = -1;
actual = g.toString().indexOf('Hi there');

reportCompare(expect, actual, summary);


