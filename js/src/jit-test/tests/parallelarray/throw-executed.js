load(libdir + "parallelarray-helpers.js");

function test() {
  var x = new Error();
  function inc(n) {
    if (inParallelSection()) 
      throw x;
    return n + 1;
  }
  var x = new ParallelArray(range(0, 2048));

  
  
  x.map(inc, {mode: "par", expect: "disqualified"});
}
test();

