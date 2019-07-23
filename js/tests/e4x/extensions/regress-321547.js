





































START("Operator .. should not implicitly quote its right operand");

var bug = 321547;
var summary = 'Operator .. should not implicitly quote its right operand';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

function a(){
  var x=<a><b><c>value c</c></b></a>;
  return x..c;
}

actual = a.toSource();
expect = 'function a() {var x = <a><b><c>value c</c></b></a>;return x..c;}';
actual = actual.replace(/[\n ]+/mg, ' ');
expect = expect.replace(/[\n ]+/mg, ' ');

TEST(2, expect, actual);

END();
