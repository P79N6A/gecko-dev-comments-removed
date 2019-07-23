





































var gTestfile = 'regress-274035.js';

var BUGNUMBER = 274035;
var summary = 'Array.prototype[concat|slice|splice] lengths';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

status = summary + ' ' + inSection(1) + ' Array.prototype.concat.length ';
expect = 1;
actual   = Array.prototype.concat.length;
reportCompare(expect, actual, status);

status = summary + ' ' + inSection(2) + ' Array.prototype.slice.length ';
expect = 2;
actual   = Array.prototype.slice.length;
reportCompare(expect, actual, status);

status = summary + ' ' + inSection(3) + ' Array.prototype.splice.length ';
expect = 2;
actual   = Array.prototype.splice.length;
reportCompare(expect, actual, status);
 
