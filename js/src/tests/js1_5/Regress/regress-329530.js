




































var gTestfile = 'regress-329530.js';

var BUGNUMBER = 329530;
var summary = 'Do not crash when calling toString on a deeply nested function';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
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
