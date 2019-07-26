







let {defer} = devtools.require("sdk/core/promise");

var gDebuggee;
var gClient;
var gTraceClient;

function run_test()
{
  initTestTracerServer();
  gDebuggee = addTestGlobal("test-tracer-actor");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestTab(gClient, "test-tracer-actor", function(aResponse, aTabClient) {
      gClient.attachTracer(aResponse.traceActor, function(aResponse, aTraceClient) {
        gTraceClient = aTraceClient;
        test_enter_exit_frame();
      });
    });
  });
  do_test_pending();
}

function test_enter_exit_frame()
{
  let packets = [];

  gTraceClient.addListener("enteredFrame", function(aEvent, aPacket) {
    do_check_eq(aPacket.type, "enteredFrame",
                'enteredFrame response should have type "enteredFrame"');
    do_check_eq(typeof aPacket.sequence, "number",
                'enteredFrame response should have sequence number');
    do_check_eq(typeof aPacket.name, "string",
                'enteredFrame response should have function name');
    do_check_eq(typeof aPacket.callsite, "object",
                'enteredFrame response should have callsite');
    do_check_eq(typeof aPacket.time, "number",
                'enteredFrame response should have time');
    packets[aPacket.sequence] = aPacket;
  });

  gTraceClient.addListener("exitedFrame", function(aEvent, aPacket) {
    do_check_eq(aPacket.type, "exitedFrame",
                'exitedFrame response should have type "exitedFrame"');
    do_check_eq(typeof aPacket.sequence, "number",
                'exitedFrame response should have sequence number');
    do_check_eq(typeof aPacket.time, "number",
                'exitedFrame response should have time');
    packets[aPacket.sequence] = aPacket;
  });

  start_trace()
    .then(eval_code)
    .then(stop_trace)
    .then(function() {
      do_check_eq(packets[2].name, "foo",
                  'Third packet in sequence should be entry to "foo" frame');

      do_check_eq(typeof packets[2].parameterNames, "object",
                  'foo entry packet should have parameterNames');
      do_check_eq(packets[2].parameterNames.length, 1,
                  'foo should have only one formal parameter');
      do_check_eq(packets[2].parameterNames[0], "x",
                  'foo should have formal parameter "x"');

      do_check_eq(typeof packets[2].arguments, "object",
                  'foo entry packet should have arguments');
      do_check_eq(typeof packets[2].arguments.values, "object",
                  'foo arguments object should have values array');
      do_check_eq(packets[2].arguments.values.length, 1,
                  'foo should have only one actual parameter');
      do_check_eq(packets[2].arguments.values[0], 42,
                  'foo should have actual parameter 42');

      do_check_eq(typeof packets[3].return, "object",
                  'Fourth packet in sequence should be exit from "foo" frame');
      do_check_eq(packets[3].return.value, "bar",
                  'foo should return "bar"');

      finishClient(gClient);
    });
}

function start_trace()
{
  let deferred = defer();
  gTraceClient.startTrace(
    ["name", "callsite", "time", "parameterNames", "arguments", "return"],
    null,
    function() { deferred.resolve(); });
  return deferred.promise;
}

function eval_code()
{
  gDebuggee.eval("(" + function() {
    function foo(x) {
      return "bar";
    }
    foo(42);
  } + ")()");
}

function stop_trace()
{
  let deferred = defer();
  gTraceClient.stopTrace(null, function() { deferred.resolve(); });
  return deferred.promise;
}
