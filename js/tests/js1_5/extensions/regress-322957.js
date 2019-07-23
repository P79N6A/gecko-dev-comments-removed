




































var bug = 322957;
var summary = 'TryMethod should not eat getter exceptions';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var obj = { get toSource() { throw "EXCEPTION"; } };

var got_proper_exception = -1;

try {
  uneval(obj);
} catch (e) {
  got_proper_exception = (e === "EXCEPTION");
}

expect = true;
actual = got_proper_exception;
reportCompare(expect, actual, summary);
