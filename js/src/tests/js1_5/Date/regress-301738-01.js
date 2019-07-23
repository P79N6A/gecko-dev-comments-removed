




































var gTestfile = 'regress-301738-01.js';

var BUGNUMBER = 301738;
var summary = 'Date parse compatibilty with MSIE';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);












var month = 'January';
var f;
var l;

f = l = 0;
expect = true;

actual = isNaN(new Date(month + ' ' + f + ' ' + l));
reportCompare(expect, actual, 'January 0 0 is invalid');

actual = isNaN(new Date(f + ' ' + l + ' ' + month));
reportCompare(expect, actual, '0 0 January is invalid');

actual = isNaN(new Date(f + ' ' + month + ' ' + l));
reportCompare(expect, actual, '0 January 0 is invalid');

f = l = 70;

actual = isNaN(new Date(month + ' ' + f + ' ' + l));
reportCompare(expect, actual, 'January 70 70 is invalid');

actual = isNaN(new Date(f + ' ' + l + ' ' + month));
reportCompare(expect, actual, '70 70 January is invalid');

actual = isNaN(new Date(f + ' ' + month + ' ' + l));
reportCompare(expect, actual, '70 January 70 is invalid');

f = 100;
l = 15;


expect = new Date(f, 0, l).toString();

actual = new Date(month + ' ' + f + ' ' + l).toString();
reportCompare(expect, actual, 'month f l');

actual = new Date(f + ' ' + l + ' ' + month).toString();
reportCompare(expect, actual, 'f l month');

actual = new Date(f + ' ' + month + ' ' + l).toString();
reportCompare(expect, actual, 'f month l');

f = 80;
l = 15;


expect = (new Date(f, 0, l)).toString();

actual = (new Date(month + ' ' + f + ' ' + l)).toString();
reportCompare(expect, actual, 'month f l');

actual = (new Date(f + ' ' + l + ' ' + month)).toString();
reportCompare(expect, actual, 'f l month');

actual = (new Date(f + ' ' + month + ' ' + l)).toString();
reportCompare(expect, actual, 'f month l');

f = 2040;
l = 15;


expect = (new Date(f, 0, l)).toString();

actual = (new Date(month + ' ' + f + ' ' + l)).toString();
reportCompare(expect, actual, 'month f l');

actual = (new Date(f + ' ' + l + ' ' + month)).toString();
reportCompare(expect, actual, 'f l month');

actual = (new Date(f + ' ' + month + ' ' + l)).toString();
reportCompare(expect, actual, 'f month l');

