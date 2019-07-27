






var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-stack");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function () {
    attachTestTabAndResume(gClient, "test-stack", function (aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      testSameBreakpoint();
    });
  });
  do_test_pending();
}


const SOURCE_URL = "http://example.com/source.js";

const testSameBreakpoint = Task.async(function* () {
  yield executeOnNextTickAndWaitForPause(evalCode, gClient);

  

  let wholeLineLocation = {
    url: SOURCE_URL,
    line: 2
  };

  let [firstResponse, firstBpClient] = yield setBreakpoint(gThreadClient, wholeLineLocation);
  let [secondResponse, secondBpClient] = yield setBreakpoint(gThreadClient, wholeLineLocation);

  do_check_eq(firstBpClient.actor, secondBpClient.actor, "Should get the same actor w/ whole line breakpoints");

  

  let columnLocation = {
    url: SOURCE_URL,
    line: 2,
    column: 6
  };

  [firstResponse, firstBpClient] = yield setBreakpoint(gThreadClient, columnLocation);
  [secondResponse, secondBpClient] = yield setBreakpoint(gThreadClient, columnLocation);

  do_check_eq(secondBpClient.actor, secondBpClient.actor, "Should get the same actor column breakpoints");

  finishClient(gClient);
});

function evalCode() {
  Components.utils.evalInSandbox(
    "" + function doStuff(k) { 
      let arg = 15;            
      k(arg);                  
    } + "\n"                   
    + "debugger;",             
    gDebuggee,
    "1.8",
    SOURCE_URL,
    1
  );
}
