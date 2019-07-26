




load(libdir + "parallelarray-helpers.js");

function iterate(x) {
  while (x == 2046) {
    
    
  }
  return 22;
}

function timeoutfunc() {
  print("Timed out, invoking the GC");
  gc();
  return false;
}

timeout(1, timeoutfunc);
new ParallelArray([2048], iterate);
