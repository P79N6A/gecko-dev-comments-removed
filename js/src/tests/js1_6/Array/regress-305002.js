




































var gTestfile = 'regress-305002.js';

var BUGNUMBER = 305002;
var summary = '[].every(f) == true';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

var notcalled = true;

function callback()
{
  notcalled = false;
}

expect = true;
actual = [].every(callback) && notcalled;

reportCompare(expect, actual, summary);
