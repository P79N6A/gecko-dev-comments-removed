

load(libdir + "parallelarray-helpers.js");

function iterate(x) {
  while (x == 2046) {
    
    
  }
  return 22;
}

timeout(1);
if (getBuildConfiguration().parallelJS)
  new ParallelArray([2048], iterate);
else
  while (true);
