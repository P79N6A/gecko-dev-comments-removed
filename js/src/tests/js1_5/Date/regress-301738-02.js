




































var gTestfile = 'regress-301738-02.js';

var BUGNUMBER = 301738;
var summary = 'Date parse compatibilty with MSIE';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);




















var f;
var m;
var l;

function newDate(f, m, l)
{
  return new Date(f + '/' + m + '/' + l);
}

function newDesc(f, m, l)
{
  return f + '/' + m + '/' + l;
}


f = 0;
m = 0;
l = 0;

expect = (new Date(l, f-1, m)).toDateString();
actual = newDate(f, m, l).toDateString();
reportCompare(expect, actual, newDesc(f, m, l));

f = 0;
m = 0;
l = 100;

expect = (new Date(l, f-1, m)).toDateString();
actual = newDate(f, m, l).toDateString();
reportCompare(expect, actual, newDesc(f, m, l));


f = 0;
m = 24;
l = 100;

expect = (new Date(l, f-1, m)).toDateString();
actual = newDate(f, m, l).toDateString();
reportCompare(expect, actual, newDesc(f, m, l));

f = 0;
m = 24;
l = 2100;

expect = (new Date(l, f-1, m)).toDateString();
actual = newDate(f, m, l).toDateString();
reportCompare(expect, actual, newDesc(f, m, l));



f = 70;
m = 24;
l = 100;

expect = (new Date(f, m-1, l)).toDateString();
actual = newDate(f, m, l).toDateString();
reportCompare(expect, actual, newDesc(f, m, l));

f = 99;
m = 12;
l = 1;

expect = (new Date(f, m-1, l)).toDateString();
actual = newDate(f, m, l).toDateString();
reportCompare(expect, actual, newDesc(f, m, l));



f = 99;
m = 70;
l = 1;

expect = true;
actual = isNaN(newDate(f, m, l));
reportCompare(expect, actual, newDesc(f, m, l) + ' is an invalid date');



f = 100;
m = 12;
l = 1;

expect = (new Date(f, m-1, l)).toDateString();
actual = newDate(f, m, l).toDateString();
reportCompare(expect, actual, newDesc(f, m, l));



f = 100;
m = 70;
l = 1;

expect = true;
actual = isNaN(newDate(f, m, l));
reportCompare(expect, actual, newDesc(f, m, l) + ' is an invalid date');


