load(libdir + "parallelarray-helpers.js");

function testReduce() {
  
  
  
  function mul(v, p) { return v*p; }

  var array = range(1, 513);
  var expected = array.reduce(mul);
  var parray = new ParallelArray(array);
  var modes = ["seq", "par"];
  for (var i = 0; i < 2; i++) {
    assertAlmostEq(expected, parray.reduce(mul, {mode: modes[i], expect: "success"}));
  }
  
  
  
  
}

testReduce();
