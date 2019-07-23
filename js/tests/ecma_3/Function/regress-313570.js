




































var bug = 313570;
var summary = 'length of objects whose prototype chain includes a function';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

function tmp() {}
tmp.prototype = function(a, b, c) {};
var obj = new tmp();


expect = 3;
actual = obj.length;
reportCompare(expect, actual, summary + ': arity');


obj.length = 10;

expect = 3;
actual = obj.length;
reportCompare(expect, actual, summary + ': immutable');

