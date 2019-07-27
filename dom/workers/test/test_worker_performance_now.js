ok(self.performance, "Performance object should exist.");
ok(typeof self.performance.now == 'function', "Performance object should have a 'now' method.");
var n = self.performance.now(), d = Date.now();
ok(n >= 0, "The value of now() should be equal to or greater than 0.");
ok(self.performance.now() >= n, "The value of now() should monotonically increase.");









var platformPossiblyLowRes;
workerTestGetOSCPU(function(oscpu) {
    platformPossiblyLowRes = oscpu.indexOf("Windows NT 5.1") == 0; 
    setTimeout(checkAfterTimeout, 1);
});
var allInts = (n % 1) == 0; 
var checks = 0;

function checkAfterTimeout() {
  checks++;
  var d2 = Date.now();
  var n2 = self.performance.now();

  allInts = allInts && (n2 % 1) == 0;
  var lowResCounter = platformPossiblyLowRes && allInts;

  if ( n2 == n && checks < 50 && 
       ( (d2 - d) < 2 
         ||
         lowResCounter &&
         (d2 - d) < 25
       )
     ) {
    setTimeout(checkAfterTimeout, 1);
    return;
  }

  
  
  ok(n2 > n, "Loose - the value of now() should increase within 2ms (or 25ms if low-res counter) (delta now(): " + (n2 - n) + " ms).");

  
  
  ok(n2 > n && (lowResCounter || checks == 1),
     "Strict - [if high-res counter] the value of now() should increase after one setTimeout (hi-res: " + (!lowResCounter) +
                                                                                              ", iters: " + checks +
                                                                                              ", dt: " + (d2 - d) +
                                                                                              ", now(): " + n2 + ").");
  workerTestDone();
};
