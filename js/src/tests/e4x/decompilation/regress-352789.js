





































gTestfile = 'regress-352789.js';

var BUGNUMBER = 352789;
var summary = 'Decompilation of new and .@';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var f;

f = function() { return new (a()).@z; };
expect = 'function() { return new (a().@z); }';
actual = f + '';
compareSource(expect, actual, inSection(1) + summary);

f = function () { return new a().@z; };
expect = 'function () { return (new a).@z; }';
actual = f + '';
compareSource(expect, actual, inSection(2) + summary);

f = function () { return (new a).@z; };
expect = 'function () { return (new a).@z; }';
actual = f + '';
compareSource(expect, actual, inSection(3) + summary);

END();
