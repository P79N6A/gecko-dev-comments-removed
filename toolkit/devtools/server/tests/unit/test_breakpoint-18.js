







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
  gDebuggee.console = { log: x => void x };
  gClient = new DebuggerClient(aServer.connectPipe());
  gClient.connect(function () {
    attachTestTabAndResume(gClient,
                           "test-breakpoints",
                           function (aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      setUpCode();
    });
  });
}

function setUpCode() {
  gClient.addOneTimeListener("paused", setBreakpoint);
  Cu.evalInSandbox(
    "debugger;\n" +
    function test() {
      console.log("foo bar");
      debugger;
    },
    gDebuggee,
    "1.8",
    "http://example.com/",
    1
  );
}

function setBreakpoint(aEvent, aPacket) {
  let source = gThreadClient.source(aPacket.frame.where.source);
  gClient.addOneTimeListener("resumed", runCode);

  source.setBreakpoint({ line: 2 }, ({ error }) => {
    do_check_true(!error);
    gThreadClient.resume();
  });
}

function runCode() {
  gClient.addOneTimeListener("paused", testBPHit);
  gDebuggee.test();
}

function testBPHit(event, { why }) {
  do_check_eq(why.type, "breakpoint");
  gClient.addOneTimeListener("paused", testDbgStatement);
  gThreadClient.resume();
}

function testDbgStatement(event, { why }) {
  
  do_check_eq(why.type, "debuggerStatement");
  
  
  do_check_neq(why.type, "breakpoint");
  gClient.close(gCallback);
}
