






var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-conditional-breakpoint");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function () {
    attachTestTabAndResume(gClient, "test-conditional-breakpoint", function (aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      test_simple_breakpoint();
    });
  });
  do_test_pending();
}

function test_simple_breakpoint()
{
  gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    let source = gThreadClient.source(aPacket.frame.where.source);
    source.setBreakpoint({
      line: 3,
      condition: "throw new Error()"
    }, function (aResponse, bpClient) {
      gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
        
        do_check_eq(aPacket.why.type, "debuggerStatement");
        do_check_eq(aPacket.frame.where.line, 4);

        
        bpClient.remove(function (aResponse) {
          gThreadClient.resume(function () {
            finishClient(gClient);
          });
        });

      });
      
      gThreadClient.resume();

    });

  });

  Components.utils.evalInSandbox("debugger;\n" +   
                                 "var a = 1;\n" +  
                                 "var b = 2;\n" +  
                                 "debugger;",      
                                 gDebuggee,
                                 "1.8",
                                 "test.js",
                                 1);
}
