
if (getBuildConfiguration().parallelJS) {
  toString = undefined;
  if (!(this in ParallelArray)) {}
} else {
  throw new TypeError();
}
