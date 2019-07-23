




































var bug = 308806;
var summary = 'Object.prototype.toLocaleString() should track Object.prototype.toString() ';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var o = {toString: function() { return 'foo'; }};

expect = o.toString();
actual = o.toLocaleString();
  
reportCompare(expect, actual, summary);
