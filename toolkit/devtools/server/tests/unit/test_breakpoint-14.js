







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
      test_simple_breakpoint();
    });
  });
  do_test_pending();
}

function test_simple_breakpoint()
{
  gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    let path = getFilePath('test_breakpoint-14.js');
    let location = { url: path, line: gDebuggee.line0 + 2};
    gThreadClient.setBreakpoint(location, function (aResponse, bpClient) {
      gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
        
        do_check_eq(aPacket.frame.where.line, gDebuggee.line0 + 5);
        do_check_eq(aPacket.why.type, "resumeLimit");

        gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
          
          do_check_eq(aPacket.frame.where.line, location.line);
          do_check_eq(aPacket.why.type, "breakpoint");
          do_check_neq(aPacket.why.type, "resumeLimit");

          gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
            
            do_check_eq(aPacket.frame.where.line, location.line);
            do_check_neq(aPacket.why.type, "breakpoint");
            do_check_eq(aPacket.why.type, "resumeLimit");
            do_check_eq(aPacket.why.frameFinished.return.type, "undefined");

            gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
              
              do_check_eq(gDebuggee.a, 1);
              do_check_eq(gDebuggee.b, undefined);
              do_check_eq(aPacket.frame.where.line, gDebuggee.line0 + 5);
              do_check_eq(aPacket.why.type, "resumeLimit");
              do_check_eq(aPacket.poppedFrames.length, 1);

              gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
                
                do_check_eq(aPacket.frame.where.line, gDebuggee.line0 + 6);
                do_check_neq(aPacket.why.type, "debuggerStatement");
                do_check_eq(aPacket.why.type, "resumeLimit");

                gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
                  
                  do_check_eq(aPacket.frame.where.line, gDebuggee.line0 + 7);
                  do_check_neq(aPacket.why.type, "debuggerStatement");
                  do_check_eq(aPacket.why.type, "resumeLimit");

                  
                  bpClient.remove(() => gThreadClient.resume(() => finishClient(gClient)));

                });
                
                gThreadClient.stepOver();
              });
              
              gThreadClient.stepOver();
            });
            
            gThreadClient.stepOver();
          });
          
          gThreadClient.stepOver();
        });
        
        gThreadClient.stepOver();
      });
      
      gThreadClient.stepOver();
    });
  });

  gDebuggee.eval("var line0 = Error().lineNumber;\n" +
                 "function foo() {\n" + 
                 "  this.a = 1;\n" +    
                 "}\n" +                
                 "debugger;\n" +        
                 "foo();\n" +           
                 "debugger;\n" +        
                 "var b = 2;\n");       
}
