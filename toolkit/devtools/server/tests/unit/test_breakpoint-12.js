







const NUM_BREAKPOINTS = 10;
var gDebuggee;
var gClient;
var gThreadClient;
var gBpActor;
var gCount;
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
  gCount = 1;
  initTestDebuggerServer(aServer);
  gDebuggee = addTestGlobal("test-stack", aServer);
  gClient = new DebuggerClient(aServer.connectPipe());
  gClient.connect(function () {
    attachTestTabAndResume(gClient, "test-stack", function (aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      test_child_skip_breakpoint();
    });
  });
}

function test_child_skip_breakpoint()
{
  gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    let source = gThreadClient.source(aPacket.frame.where.source);
    let location = { line: gDebuggee.line0 + 3};

    source.setBreakpoint(location, function (aResponse, bpClient) {
      
      do_check_eq(aResponse.actualLocation.source.actor, source.actor);
      do_check_eq(aResponse.actualLocation.line, location.line + 1);
      gBpActor = aResponse.actor;

      
      set_breakpoints(source, location);
    });

  });

  gDebuggee.eval("var line0 = Error().lineNumber;\n" +
                 "function foo() {\n" + 
                 "  this.a = 1;\n" +    
                 "  // A comment.\n" +  
                 "  this.b = 2;\n" +    
                 "}\n" +                
                 "debugger;\n" +        
                 "foo();\n");           
}


function set_breakpoints(source, location) {
  do_check_neq(gCount, NUM_BREAKPOINTS);
  source.setBreakpoint(location, function (aResponse, bpClient) {
    
    do_check_eq(aResponse.actualLocation.source.actor, source.actor);
    do_check_eq(aResponse.actualLocation.line, location.line + 1);
    
    do_check_eq(aResponse.actor, gBpActor);

    if (++gCount < NUM_BREAKPOINTS) {
      set_breakpoints(source, location);
      return;
    }

    
    
    gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
      
      do_check_eq(aPacket.type, "paused");
      do_check_eq(aPacket.frame.where.source.actor, source.actor);
      do_check_eq(aPacket.frame.where.line, location.line + 1);
      do_check_eq(aPacket.why.type, "breakpoint");
      do_check_eq(aPacket.why.actors[0], bpClient.actor);
      
      do_check_eq(gDebuggee.a, 1);
      do_check_eq(gDebuggee.b, undefined);

      gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
        
        do_check_true(false);
      });
      gThreadClient.resume(function () {
        
        do_timeout(1000, function () {
          gClient.close(gCallback);
        });
      });

    });
    
    gThreadClient.resume();
  });

}
