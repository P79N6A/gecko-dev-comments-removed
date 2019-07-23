





































var gTestfile = 'regress-245113.js';

var BUGNUMBER = 245113;
var summary = 'instanceof operator should return false for class prototype';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

Date.prototype.test = function() {
  return (this instanceof Date);
};

String.prototype.test = function() {
  return (this instanceof String);
};

status = summary + inSection(1);
expect = false;
actual = (Date.prototype.test instanceof Date);
reportCompare(expect, actual, status);

status = summary + inSection(2);
expect = false;
actual = Date.prototype.test();
reportCompare(expect, actual, status);

status = summary + inSection(3);
expect = false;
actual = String.prototype.test();
reportCompare(expect, actual, status);

status = summary + inSection(4);
expect = true;
actual = (new Date()).test();
reportCompare(expect, actual, status);

status = summary + inSection(5);
expect = true;
actual = (new String()).test();
reportCompare(expect, actual, status);

