







var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-breakpoints");
  gDebuggee.console = { log: x => void x };
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function () {
    attachTestTabAndResume(gClient,
                           "test-breakpoints",
                           function (aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      setUpCode();
    });
  });
  do_test_pending();
}

const URL = "test.js";

function setUpCode() {
  gClient.addOneTimeListener("newSource", setBreakpoint);
  Cu.evalInSandbox(
    "" + function test() {
      console.log("foo bar");
      debugger;
    },
    gDebuggee,
    "1.8",
    URL
  );
}

function setBreakpoint() {
  gThreadClient.setBreakpoint({
    url: URL,
    line: 1
  }, ({ error }) => {
    do_check_true(!error);
    gThreadClient.resume(runCode);
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
  finishClient(gClient);
}
