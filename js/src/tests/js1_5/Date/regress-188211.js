




































var gTestfile = 'regress-188211.js';

var BUGNUMBER = 188211;
var summary = 'Date.prototype.toLocaleString() error on future dates';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

var dt;

dt = new Date(208e10);
printStatus(dt+'');
expect = true;
actual = dt.toLocaleString().indexOf('2035') >= 0;
reportCompare(expect, actual, summary + ': new Date(208e10)');

dt = new Date(209e10);
printStatus(dt+'');
expect = true;
actual = dt.toLocaleString().indexOf('2036') >= 0;
reportCompare(expect, actual, summary + ': new Date(209e10)');
