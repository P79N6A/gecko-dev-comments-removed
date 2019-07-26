






var gClient;
var gThreadActor;

function run_test() {
  initTestDebuggerServer();
  let gDebuggee = addTestGlobal("test-nesting");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function () {
    attachTestTabAndResume(gClient, "test-nesting", function (aResponse, aTabClient, aThreadClient) {
      
      gThreadActor = aThreadClient._transport._serverConnection.getActor(aThreadClient._actor);

      test_nesting();
    });
  });
  do_test_pending();
}

function test_nesting() {
  const thread = gThreadActor;
  const { resolve, reject, promise: p } = promise.defer();

  
  
  
  
  
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

  do_check_eq(thread.synchronize(p), true);

  
  do_check_eq(++currentStep, 4);
  
  do_check_eq(thread._nestedEventLoops.size, 0);

  finishClient(gClient);
}
