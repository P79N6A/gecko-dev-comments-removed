

function t() {
  var x = [0, 1, 2];
  return [for (x of x) x*2]
}
assertDeepEq(t(), [0, 2, 4]);



function t2() {
  var x = [0, 1, 2];
  return [for (x of x) ()=>x]
}

assertDeepEq([for (x of t2()) x()], [2, 2, 2]);

reportCompare(null, null, "test");
