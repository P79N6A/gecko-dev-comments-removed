


var gTestfile = 'parse-arguments.js';

var BUGNUMBER = 653847;
var summary = "JSON.parse handling of omitted arguments";

print(BUGNUMBER + ": " + summary);





try
{
  var r = JSON.parse();
  throw new Error("didn't throw, returned " + r);
}
catch (e)
{
  assertEq(e instanceof SyntaxError, true, "expected syntax error, got: " + e);
}



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
