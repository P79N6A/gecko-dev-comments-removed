





































var bug = 352459;
var summary = 'decompilation for 4..@x++';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var f = (function() { return 4..@x++; } );
expect = 'function() { return (4).@x++; }';
actual = f + '';
compareSource(1, expect, actual);

END();
