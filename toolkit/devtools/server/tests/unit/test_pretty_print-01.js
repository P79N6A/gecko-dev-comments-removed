



var gDebuggee;
var gClient;
var gThreadClient;



function run_test() {
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-pretty-print");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestTabAndResume(gClient, "test-pretty-print", function(aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      evalCode();
    });
  });
  do_test_pending();
}

function evalCode() {
  gClient.addOneTimeListener("newSource", prettyPrintSource);
  const code = "" + function main() { let a = 1 + 3; let b = a++; return b + a; };
  Cu.evalInSandbox(
    code,
    gDebuggee,
    "1.8",
    "data:text/javascript," + code);
}

function prettyPrintSource(event, { source }) {
  gThreadClient.source(source).prettyPrint(4, testPrettyPrinted);
}

function testPrettyPrinted({ error, source}) {
  do_check_true(!error);
  do_check_true(source.contains("\n    "));
  finishClient(gClient);
}
