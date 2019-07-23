





































gTestfile = 'regress-321547.js';

var summary = "Operator .. should not implicitly quote its right operand";
var BUGNUMBER = 321547;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

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
