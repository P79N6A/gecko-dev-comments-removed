





































var bug = 352789;
var summary = 'Decompilation of new and .@';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var f;

f = function() { return new (a()).@z; };
expect = 'function() { return new (a().@z); }';
actual = f + '';
compareSource(1, expect, actual);

f = function () { return new a().@z; };
expect = 'function () { return (new a).@z; }';
actual = f + '';
compareSource(1, expect, actual);

f = function () { return (new a).@z; };
expect = 'function () { return (new a).@z; }';
actual = f + '';
compareSource(1, expect, actual);

END();
