







var gDebuggee;
var gClient;
var gTraceClient;
var gThreadClient;

Components.utils.import('resource:///modules/devtools/SourceMap.jsm');

function run_test()
{
  initTestTracerServer();
  gDebuggee = addTestGlobal("test-tracer-actor");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestThread(gClient, "test-tracer-actor",
      function(aResponse, aTabClient, aThreadClient, aTabResponse) {
      gThreadClient = aThreadClient;
      gThreadClient.resume(function (aResponse) {
        gClient.attachTracer(aTabResponse.traceActor,
          function(aResponse, aTraceClient) {
          gTraceClient = aTraceClient;
          testTraces();
        });
      });
    });
  });
  do_test_pending();
}

const testTraces = Task.async(function* () {
  
  const tracesStopped = promise.defer();
  gClient.addListener("traces", (aEvent, { traces }) => {
    for (let t of traces) {
      check_trace(t);
    }
    tracesStopped.resolve();
  });

  yield startTrace();

  yield executeOnNextTickAndWaitForPause(evalSourceMapSetup, gClient);
  yield resume(gThreadClient);

  evalSetup();

  evalTestCode();

  yield tracesStopped.promise;
  yield stopTrace();

  finishClient(gClient);
});

function startTrace()
{
  let deferred = promise.defer();
  gTraceClient.startTrace(["depth", "name", "location"], null,
    function() { deferred.resolve(); });
  return deferred.promise;
}

function evalSourceMapSetup() {
  let { code, map } = (new SourceNode(null, null, null, [
    new SourceNode(1, 0, "b_original.js", "" + function fnSourceMapped() {
      fnInner();
    } + "\n debugger;")
  ])).toStringWithSourceMap({
    file: "b.js",
    sourceRoot: "http://example.com/"
  });
  code += "//# sourceMappingURL=data:text/json," + map.toString();
  Components.utils.evalInSandbox(code, gDebuggee, "1.8", "http://example.com/b.js");
}

function evalSetup()
{
  Components.utils.evalInSandbox(
    "" + function fnOuter() {
      fnSourceMapped();
    } + "\n" +
    "" + function fnInner() {
      [1].forEach(function noop() {});
    },
    gDebuggee,
    "1.8",
    "http://example.com/a.js",
    1
  );
}

function evalTestCode()
{
  Components.utils.evalInSandbox(
    "fnOuter();",
    gDebuggee,
    "1.8",
    "http://example.com/a.js",
    1
  );
}

function stopTrace()
{
  let deferred = promise.defer();
  gTraceClient.stopTrace(null, function() { deferred.resolve(); });
  return deferred.promise;
}

function check_trace({ type, sequence, depth, name, location, blackBoxed })
{
  switch(sequence) {
  
  
  case 0:
  case 2:
  case 4:
    do_check_eq(name, "(global)");
    do_check_eq(type, "enteredFrame");
    break;

  case 5:
    do_check_eq(name, "fnOuter");
    break;

  case 6:
    do_check_eq(name, "fnSourceMapped");
    break;

  case 7:
    do_check_eq(name, "fnInner");
    break;

  case 8:
    do_check_eq(name, "noop");
    break;

  case 1: 
  case 3: 
  case 9: 
  case 10: 
  case 11: 
  case 12: 
  case 13: 
    do_check_eq(type, "exitedFrame");
    break;

  default:
    
    do_check_true(false);
  }
}
