





































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

actual = a.toString();
expect = 'function a() {\n' + 
         '    var x = <a><b><c>value c</c></b></a>;\n' +
         '    return x..c;\n' +
         '}';
actual = actual.replace(/[\n ]+/mg, ' ');
expect = expect.replace(/[\n ]+/mg, ' ');

TEST(1, expect, actual);

actual = String(a.valueOf());
expect = 'function a() {\n' + 
         '    var x = <a><b><c>value c</c></b></a>;\n' +
         '    return x..c;\n' +
         '}';
actual = actual.replace(/[\n ]+/mg, ' ');
expect = expect.replace(/[\n ]+/mg, ' ');

TEST(3, expect, actual);

actual = String(a);
expect = 'function a() {\n' + 
         '    var x = <a><b><c>value c</c></b></a>;\n' +
         '    return x..c;\n' +
         '}';
actual = actual.replace(/[\n ]+/mg, ' ');
expect = expect.replace(/[\n ]+/mg, ' ');

TEST(4, expect, actual);

actual = a();
expect = <c>value c</c>;
actual = actual.replace(/[\n ]+/mg, ' ');
expect = expect.replace(/[\n ]+/mg, ' ');

TEST(5, expect, actual);

END();
