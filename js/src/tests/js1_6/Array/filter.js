




































var gTestfile = 'filter.js';

var BUGNUMBER     = "364603";
var summary = "The value placed in a filtered array for an element is the " +
  " element's value before the callback is run, not after";
var actual, expect;

printBugNumber(BUGNUMBER);
printStatus(summary);





var failed = false;

function mutate(val, index, arr)
{
  arr[index] = "mutated";
  return true;
}

function assertEqual(v1, v2, msg)
{
  if (v1 !== v2)
    throw msg;
}

try
{
  var a = [1, 2];
  var m = a.filter(mutate);

  assertEqual(a[0], "mutated", "Array a not mutated!");
  assertEqual(a[1], "mutated", "Array a not mutated!");

  assertEqual(m[0], 1, "Filtered value is value before callback is run");
  assertEqual(m[1], 2, "Filtered value is value before callback is run");
}
catch (e)
{
  failed = e;
}


expect = false;
actual = failed;

reportCompare(expect, actual, summary);
