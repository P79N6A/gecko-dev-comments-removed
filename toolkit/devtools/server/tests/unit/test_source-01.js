



var gDebuggee;
var gClient;
var gThreadClient;





function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-grips");
  Cu.evalInSandbox(
    "" + function stopMe(arg1) {
      debugger;
    },
    gDebuggee,
    "1.8",
    getFileUrl("test_source-01.js")
  );

  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestTabAndResume(gClient, "test-grips", function(aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      test_source();
    });
  });
  do_test_pending();
}

const SOURCE_URL = "http://example.com/foobar.js";
const SOURCE_CONTENT = "stopMe()";

function test_source()
{
  DebuggerServer.LONG_STRING_LENGTH = 200;

  gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket) {
    gThreadClient.getSources(function (aResponse) {
      do_check_true(!!aResponse);
      do_check_true(!!aResponse.sources);

      let source = aResponse.sources.filter(function (s) {
        return s.url === SOURCE_URL;
      })[0];

      do_check_true(!!source);

      let sourceClient = gThreadClient.source(source);
      sourceClient.source(function (aResponse) {
        do_check_true(!!aResponse);
        do_check_true(!aResponse.error);
        do_check_true(!!aResponse.contentType);
        do_check_true(aResponse.contentType.includes("javascript"));

        do_check_true(!!aResponse.source);
        do_check_eq(SOURCE_CONTENT,
                    aResponse.source);

        gThreadClient.resume(function () {
          finishClient(gClient);
        });
      });
    });
  });

  Cu.evalInSandbox(
    SOURCE_CONTENT,
    gDebuggee,
    "1.8",
    SOURCE_URL
  );
}
