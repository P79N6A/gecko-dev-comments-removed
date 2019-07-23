





































gTestfile = 'regress-352459.js';

var BUGNUMBER = 352459;
var summary = 'decompilation for 4..@x++';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var f = (function() { return 4..@x++; } );
expect = 'function() { return (4).@x++; }';
actual = f + '';
compareSource(expect, actual, inSection(1) + summary);

END();
