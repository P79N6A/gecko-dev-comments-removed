function bug854050() {
  
  
  for (z = 0; z < 1; z++)
    function x(b, x) {}
  ParallelArray(47, x);
}

bug854050();
