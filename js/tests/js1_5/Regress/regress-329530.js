




































var bug = 329530;
var summary = 'Do not crash when calling toString on a deeply nested function';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

expectExitCode(0);
expectExitCode(5);

var nestingLevel = 1000;

function buildTestFunction(N) {
  var i, funSourceStart = "", funSourceEnd = "";
  for (i=0; i < N; ++i) {
    funSourceStart += "function testFoo() {";
    funSourceEnd += "}";
  }
  return Function(funSourceStart + funSourceEnd);
}

try
{
  var testSource = buildTestFunction(nestingLevel).toString();
  printStatus(testSource.length);
}
catch(ex)
{
  printStatus(ex + '');
}

  
reportCompare(expect, actual, summary);
