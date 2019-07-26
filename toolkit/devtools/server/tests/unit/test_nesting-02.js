






const { defer } = devtools.require("sdk/core/promise");

function run_test() {
  initTestDebuggerServer();
  do_test_pending();
  test_nesting();
}

function test_nesting() {
  const thread = new DebuggerServer.ThreadActor(DebuggerServer);
  const { resolve, reject, promise } = defer();

  
  
  
  
  
  let currentStep = 0;

  executeSoon(function () {
    let eventLoop;

    executeSoon(function () {
      
      do_check_eq(++currentStep, 2);
      
      do_check_eq(thread._nestedEventLoops.size, 2);

      executeSoon(function () {
        
        do_check_eq(++currentStep, 3);
        
        
        do_check_eq(thread._nestedEventLoops.size, 2);
        
        do_check_true(!!eventLoop);
        eventLoop.resolve();
      });

      resolve(true);
      
      do_check_eq(thread._nestedEventLoops.size, 2);
    });

    
    do_check_eq(++currentStep, 1);
    
    do_check_eq(thread._nestedEventLoops.size, 1);
    eventLoop = thread._nestedEventLoops.push();
    eventLoop.enter();
  });

  do_check_eq(thread.synchronize(promise), true);

  
  do_check_eq(++currentStep, 4);
  
  do_check_eq(thread._nestedEventLoops.size, 0);

  do_test_finished();
}
