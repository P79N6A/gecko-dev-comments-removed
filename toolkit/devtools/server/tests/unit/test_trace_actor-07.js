






let { defer } = devtools.require("sdk/core/promise");

var gDebuggee;
var gClient;
var gTraceClient;

function run_test()
{
  initTestTracerServer();
  gDebuggee = addTestGlobal("test-tracer-actor");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestTab(gClient, "test-tracer-actor", function(aResponse, aTabClient) {
      gClient.attachTracer(aResponse.traceActor, function(aResponse, aTraceClient) {
        gTraceClient = aTraceClient;
        test_exit_frame_whys();
      });
    });
  });
  do_test_pending();
}

function test_exit_frame_whys()
{
  gTraceClient.addListener("exitedFrame", check_packet);

  start_trace()
    .then(eval_code)
    .then(stop_trace)
    .then(function() {
      finishClient(gClient);
    }).then(null, error => {
      do_check_true(false, "Should not get an error, got: " + error);
    });
}

function start_trace()
{
  let deferred = defer();
  gTraceClient.startTrace(["name"], null, function() { deferred.resolve(); });
  return deferred.promise;
}

function eval_code()
{
  gDebuggee.eval("(" + function() {
    function thrower() {
      throw new Error();
    }

    function* yielder() {
      yield 1;
    }

    function returner() {
      return 1;
    }

    try {
      thrower();
    } catch(e) {}

    
    
    
    

    returner();
  } + ")()");
}

function stop_trace()
{
  let deferred = defer();
  gTraceClient.stopTrace(null, function() { deferred.resolve(); });
  return deferred.promise;
}

function check_packet(aEvent, { sequence, why })
{
  switch(sequence) {
  case 3:
    do_check_eq(why, "throw");
    break;
  case 5:
    do_check_eq(why, "return");
    break;
  }
}
