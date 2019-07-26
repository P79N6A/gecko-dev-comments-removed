






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
      
      
      
      Services.tm.mainThread.dispatch({
        run: test_simple_stepping
      }, Ci.nsIThread.DISPATCH_NORMAL);
    });
  });
  do_test_pending();
}

function test_simple_stepping()
{
  gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
      
      do_check_eq(aPacket.type, "paused");
      do_check_eq(aPacket.frame.where.line, gDebuggee.line0 + 4);
      do_check_eq(aPacket.why.type, "resumeLimit");
      do_check_eq(aPacket.why.frameFinished.return, 10);

      gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
        gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
          
          do_check_eq(aPacket.type, "paused");
          do_check_eq(aPacket.frame.where.line, gDebuggee.line0 + 7);
          do_check_eq(aPacket.why.type, "resumeLimit");
          do_check_eq(aPacket.why.frameFinished.return.type, "undefined");

          gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
            gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
              
              do_check_eq(aPacket.type, "paused");
              do_check_eq(aPacket.frame.where.line, gDebuggee.line0 + 12);
              do_check_eq(aPacket.why.type, "resumeLimit");
              do_check_eq(aPacket.why.frameFinished.throw, "ah");

              gThreadClient.resume(function () {
                finishClient(gClient);
              });
            });
            gThreadClient.stepOut();
          });
          gThreadClient.resume();
        });
        gThreadClient.stepOut();
      });
      gThreadClient.resume();
    });
    gThreadClient.stepOut();

  });

  gDebuggee.eval("var line0 = Error().lineNumber;\n" +
                 "function f() {\n" + 
                 "  debugger;\n" +    
                 "  var a = 10;\n" +  
                 "  return a;\n" +    
                 "}\n" +              
                 "function g() {\n" + 
                 "  debugger;\n" +    
                 "}\n" +              
                 "function h() {\n" + 
                 "  debugger;\n" +    
                 "  throw 'ah';\n" +  
                 "  return 2;\n" +    
                 "}\n" +              
                 "f();\n" +           
                 "g();\n" +           
                 "h();\n");           
}
