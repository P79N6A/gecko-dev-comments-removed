






const { defer } = devtools.require("sdk/core/promise");
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
  const { resolve, reject, promise } = defer();

  let currentStep = 0;

  executeSoon(function () {
    
    do_check_eq(++currentStep, 1);
    
    do_check_eq(thread._nestedEventLoops.size, 1);
    resolve(true);
  });

  do_check_eq(thread.synchronize(promise), true);

  
  do_check_eq(++currentStep, 2);
  
  do_check_eq(thread._nestedEventLoops.size, 0);

  finishClient(gClient);
}
