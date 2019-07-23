





































gTestfile = 'regress-373678.js';

var BUGNUMBER = 373678;
var summary = 'Missing quotes around string in decompilation, ' +
    'with for..in and do..while';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var f = function() { do {for(a.b in []) { } } while("c\\d"); };
expect = 'function() { do {for(a.b in []) { } } while("c\\\\d"); }';
actual = f + '';
compareSource(expect, actual, inSection(1) + summary);

END();
