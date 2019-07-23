





































var bug = 281930;
var summary = 'this reference should point to global object in function expressions';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var global = this;

actual = function() { return this; }();
expect = global;
  
reportCompare(expect, actual, summary);
