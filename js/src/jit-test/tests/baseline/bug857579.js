

function testMonitorIntrinsic() {
  var N = 2;
  
  var p = new ParallelArray([N,N], function () 0);
  
  for (var i = 0; i < N+1; i++) {
    for (var j = 0; j < 2; j++) {
      
      
      
      p.get(i).get(j);
    }
  }
}

if (getBuildConfiguration().parallelJS) {
  testMonitorIntrinsic();
} else {
  throw new TypeError();
}
