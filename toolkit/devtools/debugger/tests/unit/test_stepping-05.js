








var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-stack");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function () {
    attachTestGlobalClientAndResume(gClient,
                                    "test-stack",
                                    function (aResponse, aThreadClient) {
      gThreadClient = aThreadClient;
      test_stepping_last();
    });
  });
  do_test_pending();
}

function test_stepping_last()
{
  gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
      
      do_check_eq(aPacket.type, "paused");
      do_check_eq(aPacket.frame.where.line, gDebuggee.line0 + 2);
      do_check_eq(aPacket.why.type, "resumeLimit");
      
      do_check_eq(gDebuggee.a, undefined);
      do_check_eq(gDebuggee.b, undefined);

      gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
        
        do_check_eq(aPacket.type, "paused");
        do_check_eq(aPacket.frame.where.line, gDebuggee.line0 + 3);
        do_check_eq(aPacket.why.type, "resumeLimit");
        
        do_check_eq(gDebuggee.a, 1);
        do_check_eq(gDebuggee.b, undefined);

        gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
          
          do_check_eq(aPacket.type, "paused");
          
          do_check_eq(aPacket.frame.where.line, gDebuggee.line0 + 3);
          do_check_eq(aPacket.why.type, "resumeLimit");
          
          do_check_eq(gDebuggee.a, 1);
          do_check_eq(gDebuggee.b, 2);

          gThreadClient.stepIn(function () {
            test_next_pause();
          });
        });
        gThreadClient.stepIn();
      });
      gThreadClient.stepIn();

    });
    gThreadClient.stepIn();

  });

  gDebuggee.eval("var line0 = Error().lineNumber;\n" +
                 "debugger;\n" +   
                 "var a = 1;\n" +  
                 "var b = 2;\n");  
}

function test_next_pause()
{
  gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    
    do_check_eq(aPacket.type, "paused");
    
    do_check_eq(aPacket.why.type, "debuggerStatement");

    gThreadClient.resume(function () {
      finishClient(gClient);
    });
  });

  gDebuggee.eval("debugger;");
}
