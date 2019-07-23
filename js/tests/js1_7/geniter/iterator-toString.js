




































var bug     = "(none)";
var summary = "gen.toString() omitting 'yield' from value";
var actual, expect;

printBugNumber(bug);
printStatus(summary);





var failed = false;

function gen()
{
  yield 17;
}

try
{
  var str = gen.toString();
  var index = str.search(/yield/);

  if (index < 0)
    throw "yield not found in str: " + str;
}
catch (e)
{
  failed = e;
}



expect = false;
actual = failed;

reportCompare(expect, actual, summary);
