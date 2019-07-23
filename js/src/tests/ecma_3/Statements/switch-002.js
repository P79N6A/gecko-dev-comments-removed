






var x = undefined;
var passed = false;
switch (x)
{
  case undefined:
    passed = true;
  default:
    break;
}
assertEq(passed, true);

reportCompare(true, true);
