





if (getBuildConfiguration().parallelJS) {
  var p = new ParallelArray([1,25e8 ,3,4]);
  var pp = p.partition(.34 );
} else {
  throw new Error();
}
