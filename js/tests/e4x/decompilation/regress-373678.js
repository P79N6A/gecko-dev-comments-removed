





































var bug = 373678;
var summary = 'Missing quotes around string in decompilation, with for..in and do..while ';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var f = function() { do {for(a.b in []) { } } while("c\\d"); };
expect = 'function() { do {for(a.b in []) { } } while("c\\\\d"); }';
actual = f + '';
compareSource(1, expect, actual);

END();
