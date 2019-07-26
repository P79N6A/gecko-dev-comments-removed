


var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-grips");

  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestGlobalClientAndResume(gClient, "test-grips", function(aResponse, aThreadClient) {
      gThreadClient = aThreadClient;
      test_banana_environment();
    });
  });
  do_test_pending();
}

function test_banana_environment()
{

  gThreadClient.addOneTimeListener("paused",
    function(aEvent, aPacket) {
      do_check_matches({type:"paused", frame:
                        {environment:
                         {type: "function", function: {name: "banana3"},
                          parent:
                          {type: "block", bindings: {variables: {banana3:undefined}},
                           parent:
                           {type: "function", function: {name: "banana2"},
                            parent:
                            {type:"block", bindings: {variables: {banana2:undefined}},
                             parent:
                             {type:"block", bindings: {variables: {banana2:undefined}},
                              parent:
                              {type:"function", function: {name: "banana"}}}}}}}}},
                       aPacket,
                       { Object:Object, Array:Array });
      gThreadClient.resume(function () {
                             finishClient(gClient);
                           });
    });

  gDebuggee.eval("\
        function banana(x) {                                            \n\
          return function banana2(y) {                                  \n\
            return function banana3(z) {                                \n\
              debugger;                                                 \n\
            };                                                          \n\
          };                                                            \n\
        }                                                               \n\
        banana('x')('y')('z');                                          \n\
        ");
}
