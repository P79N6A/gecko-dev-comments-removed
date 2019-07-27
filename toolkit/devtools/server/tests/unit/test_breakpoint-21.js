








var gDebuggee;
var gClient;
var gThreadClient;
var gCallback;

function run_test()
{
  run_test_with_server(DebuggerServer, function () {
    run_test_with_server(WorkerDebuggerServer, do_test_finished);
  });
  do_test_pending();
};

function run_test_with_server(aServer, aCallback)
{
  gCallback = aCallback;
  initTestDebuggerServer(aServer);
  gDebuggee = addTestGlobal("test-breakpoints", aServer);
  gClient = new DebuggerClient(aServer.connectPipe());
  gClient.connect(function () {
    attachTestTabAndResume(gClient,
                           "test-breakpoints",
                           function (aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      test();
    });
  });
}

const test = Task.async(function*() {
  
  
  yield getSources(gThreadClient);

  let packet = yield executeOnNextTickAndWaitForPause(evalCode, gClient);
  let source = gThreadClient.source(packet.frame.where.source);
  let location = {
    line: gDebuggee.line0 + 8
  };

  let [res, bpClient] = yield setBreakpoint(source, location);
  ok(!res.error);

  yield resume(gThreadClient);
  packet = yield waitForPause(gClient);
  do_check_eq(packet.type, "paused");
  do_check_eq(packet.why.type, "breakpoint");
  do_check_eq(packet.why.actors[0], bpClient.actor);
  do_check_eq(packet.frame.where.source.actor, source.actor);
  do_check_eq(packet.frame.where.line, location.line);

  yield resume(gThreadClient);
  finishClient(gClient);
});

function evalCode() {
  
  Components.utils.evalInSandbox(
    "var line0 = Error().lineNumber;\n(" + function() {
      debugger;
      var a = (function() {
        return (function() {
          return (function() {
            return (function() {
              return (function() {
                var x = 10; 
                return 1;
              })();
            })();
          })();
        })();
      })();
    } + ")()",
    gDebuggee
  );
}
