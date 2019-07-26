









const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DeferredTask",
                                  "resource://gre/modules/DeferredTask.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");







const T = 100;




function promiseTimeout(aTimeoutMs)
{
  let deferred = Promise.defer();
  do_timeout(aTimeoutMs, deferred.resolve);
  return deferred.promise;
}

function run_test()
{
  run_next_test();
}







add_test(function test_arm_simple()
{
  new DeferredTask(run_next_test, 10).arm();
});




add_test(function test_arm_delay_respected()
{
  let executed1 = false;
  let executed2 = false;

  new DeferredTask(function () {
    executed1 = true;
    do_check_false(executed2);
  }, 1*T).arm();

  new DeferredTask(function () {
    executed2 = true;
    do_check_true(executed1);
    run_next_test();
  }, 2*T).arm();
});




add_test(function test_arm_delay_notrestarted()
{
  let executed = false;

  
  let deferredTask = new DeferredTask(() => { executed = true; }, 4*T);
  deferredTask.arm();

  
  do_timeout(2*T, () => deferredTask.arm());

  
  do_timeout(5*T, function () {
    do_check_true(executed);
    run_next_test();
  });
});




add_test(function test_arm_coalesced()
{
  let executed = false;

  let deferredTask = new DeferredTask(function () {
    do_check_false(executed);
    executed = true;
    run_next_test();
  }, 50);

  deferredTask.arm();
  deferredTask.arm();
});





add_test(function test_arm_coalesced_nodelay()
{
  let executed = false;

  let deferredTask = new DeferredTask(function () {
    do_check_false(executed);
    executed = true;
    run_next_test();
  }, 0);

  deferredTask.arm();
  deferredTask.arm();
});




add_test(function test_arm_recursive()
{
  let executed = false;

  let deferredTask = new DeferredTask(function () {
    if (!executed) {
      executed = true;
      deferredTask.arm();
    } else {
      run_next_test();
    }
  }, 50);

  deferredTask.arm();
});





add_test(function test_arm_async()
{
  let finishedExecution = false;
  let finishedExecutionAgain = false;

  
  let deferredTask = new DeferredTask(function () {
    yield promiseTimeout(4*T);
    if (!finishedExecution) {
      finishedExecution = true;
    } else if (!finishedExecutionAgain) {
      finishedExecutionAgain = true;
    }
  }, 2*T);
  deferredTask.arm();

  
  
  
  do_timeout(4*T, function () {
    do_check_true(deferredTask.isRunning);
    do_check_false(finishedExecution);
    deferredTask.arm();
  });

  
  
  do_timeout(7*T, function () {
    do_check_false(deferredTask.isRunning);
    do_check_true(finishedExecution);
  });

  
  do_timeout(10*T, function () {
    do_check_true(deferredTask.isRunning);
    do_check_false(finishedExecutionAgain);
  });

  
  do_timeout(13*T, function () {
    do_check_false(deferredTask.isRunning);
    do_check_true(finishedExecutionAgain);
    run_next_test();
  });
});




add_test(function test_disarm()
{
  
  let deferredTask = new DeferredTask(function () {
    do_throw("This task should not run.");
  }, 2*T);
  deferredTask.arm();

  
  do_timeout(1*T, () => deferredTask.disarm());

  
  do_timeout(3*T, run_next_test);
});




add_test(function test_disarm_delay_restarted()
{
  let executed = false;

  let deferredTask = new DeferredTask(() => { executed = true; }, 4*T);
  deferredTask.arm();

  do_timeout(2*T, function () {
    deferredTask.disarm();
    deferredTask.arm();
  });

  do_timeout(5*T, function () {
    do_check_false(executed);
  });

  do_timeout(7*T, function () {
    do_check_true(executed);
    run_next_test();
  });
});





add_test(function test_disarm_async()
{
  let finishedExecution = false;

  let deferredTask = new DeferredTask(function () {
    deferredTask.arm();
    yield promiseTimeout(2*T);
    finishedExecution = true;
  }, 1*T);
  deferredTask.arm();

  do_timeout(2*T, function () {
    do_check_true(deferredTask.isRunning);
    do_check_true(deferredTask.isArmed);
    do_check_false(finishedExecution);
    deferredTask.disarm();
  });

  do_timeout(4*T, function () {
    do_check_false(deferredTask.isRunning);
    do_check_false(deferredTask.isArmed);
    do_check_true(finishedExecution);
    run_next_test();
  });
});





add_test(function test_disarm_immediate_async()
{
  let executed = false;

  let deferredTask = new DeferredTask(function () {
    do_check_false(executed);
    executed = true;
    yield promiseTimeout(2*T);
  }, 1*T);
  deferredTask.arm();

  do_timeout(2*T, function () {
    do_check_true(deferredTask.isRunning);
    do_check_false(deferredTask.isArmed);
    deferredTask.arm();
    deferredTask.disarm();
  });

  do_timeout(4*T, function () {
    do_check_true(executed);
    do_check_false(deferredTask.isRunning);
    do_check_false(deferredTask.isArmed);
    run_next_test();
  });
});




add_test(function test_isArmed_isRunning()
{
  let deferredTask = new DeferredTask(function () {
    do_check_true(deferredTask.isRunning);
    do_check_false(deferredTask.isArmed);
    deferredTask.arm();
    do_check_true(deferredTask.isArmed);
    deferredTask.disarm();
    do_check_false(deferredTask.isArmed);
    run_next_test();
  }, 50);

  do_check_false(deferredTask.isArmed);
  deferredTask.arm();
  do_check_true(deferredTask.isArmed);
  do_check_false(deferredTask.isRunning);
});




add_test(function test_finalize()
{
  let executed = false;
  let timePassed = false;

  let deferredTask = new DeferredTask(function () {
    do_check_false(timePassed);
    executed = true;
  }, 2*T);
  deferredTask.arm();

  do_timeout(1*T, () => { timePassed = true; });

  
  deferredTask.finalize().then(function () {
    do_check_true(executed);
    run_next_test();
  });
});





add_test(function test_finalize_executes_entirely()
{
  let executed = false;
  let executedAgain = false;
  let timePassed = false;

  let deferredTask = new DeferredTask(function () {
    
    if (!executed) {
      deferredTask.arm();
      do_check_true(deferredTask.isArmed);
      do_check_true(deferredTask.isRunning);

      deferredTask.finalize().then(function () {
        
        do_check_true(executedAgain);
        do_check_false(timePassed);
        do_check_false(deferredTask.isArmed);
        do_check_false(deferredTask.isRunning);
        run_next_test();
      });

      
      
      
      
      
      do_timeout(3*T, () => { timePassed = true; });
    }

    yield promiseTimeout(1*T);

    
    if (executed) {
      do_check_true(deferredTask.isRunning);
      executedAgain = true;
    } else {
      executed = true;
    }
  }, 2*T);

  deferredTask.arm();
});
