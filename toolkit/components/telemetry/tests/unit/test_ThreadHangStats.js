


Cu.import("resource://gre/modules/Services.jsm");

function getMainThreadHangStats() {
  let threads = Services.telemetry.threadHangStats;
  return threads.find((thread) => (thread.name === "Gecko"));
}

function run_test() {
  let startHangs = getMainThreadHangStats();

  
  
  
  if (!startHangs) {
    ok("Hang reporting not enabled.");
    return;
  }

  if (Services.appinfo.OS === 'Linux' || Services.appinfo.OS === 'Android') {
    
    
    
    let kernel = Services.sysinfo.get('kernel_version') ||
                 Services.sysinfo.get('version');
    if (Services.vc.compare(kernel, '2.6.31') < 0) {
      ok("Hang reporting not supported for old kernel.");
      return;
    }
  }

  
  
  
  

  do_execute_soon(() => {
    
    let startTime = Date.now();
    while ((Date.now() - startTime) < 1000) {
    }
  });

  do_execute_soon(() => {
    
    let startTime = Date.now();
    while ((Date.now() - startTime) < 10000) {
    }
  });

  do_execute_soon(() => {
    do_test_pending();

    let check_results = () => {
      let endHangs = getMainThreadHangStats();

      
      
      
      if (endHangs.hangs.length === startHangs.hangs.length) {
        do_timeout(100, check_results);
        return;
      }

      let check_histogram = (histogram) => {
        equal(typeof histogram, "object");
        equal(histogram.histogram_type, 0);
        equal(typeof histogram.min, "number");
        equal(typeof histogram.max, "number");
        equal(typeof histogram.sum, "number");
        ok(Array.isArray(histogram.ranges));
        ok(Array.isArray(histogram.counts));
        equal(histogram.counts.length, histogram.ranges.length);
      };

      
      equal(typeof endHangs, "object");
      check_histogram(endHangs.activity);

      ok(Array.isArray(endHangs.hangs));
      notEqual(endHangs.hangs.length, 0);

      ok(Array.isArray(endHangs.hangs[0].stack));
      notEqual(endHangs.hangs[0].stack.length, 0);
      equal(typeof endHangs.hangs[0].stack[0], "string");

      
      
      ok(endHangs.hangs.some((hang) => (
        Array.isArray(hang.nativeStack) &&
        hang.nativeStack.length !== 0 &&
        typeof hang.nativeStack[0] === "string"
      )));

      check_histogram(endHangs.hangs[0].histogram);

      do_test_finished();
    };

    check_results();
  });
}
