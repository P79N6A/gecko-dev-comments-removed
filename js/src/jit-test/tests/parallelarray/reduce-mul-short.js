load(libdir + "parallelarray-helpers.js");

function testReduce() {
  
  
  

  function mul(v, p) { return v*p; }

  
  var array = range(1, 513).map(function(v) { return (v % 4) + 1; });
  var expected = array.reduce(mul);
  print(expected);

  var parray = new ParallelArray(array);
  var results = ["success", "mixed", "success", "success"];
  var actual;
  for (var i = 0; i < results.length; i++) {
    print("Iteration ", i, " expects ", results[i]);
    actual = parray.reduce(mul, {mode: "par", expect: results[i]});
  }

  assertAlmostEq(actual, expected);
}

testReduce();
