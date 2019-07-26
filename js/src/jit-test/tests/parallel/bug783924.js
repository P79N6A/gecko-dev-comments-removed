function bug783924() {
  
  Function("ParallelArray([])")();
}

if (getBuildConfiguration().parallelJS)
  bug783924();
