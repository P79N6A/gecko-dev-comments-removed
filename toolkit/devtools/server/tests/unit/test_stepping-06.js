






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
  gDebuggee = addTestGlobal("test-stack", aServer);
  gClient = new DebuggerClient(aServer.connectPipe());
  gClient.connect(function () {
    attachTestTabAndResume(gClient, "test-stack", function (aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      
      
      
      Services.tm.mainThread.dispatch({
        run: test_simple_stepping
      }, Ci.nsIThread.DISPATCH_NORMAL);
    });
  });
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
              do_check_eq(aPacket.frame.where.line, gDebuggee.line0 + 11);
              do_check_eq(aPacket.why.type, "resumeLimit");
              do_check_eq(aPacket.why.frameFinished.throw, "ah");

              gThreadClient.resume(function () {
                gClient.close(gCallback);
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
                 "try { h() } catch (ex) { };\n");      
}
