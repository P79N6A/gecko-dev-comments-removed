



var gDebuggee;
var gClient;
var gThreadClient;





Cu.import("resource://gre/modules/NetUtil.jsm");

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-grips");
  gDebuggee.eval(function stopMe(arg1) {
    debugger;
  }.toString());

  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestGlobalClientAndResume(gClient, "test-grips", function(aResponse, aThreadClient) {
      gThreadClient = aThreadClient;
      gThreadClient.addListener("unsolicitedPause", unsolicitedPauseListener);
      test_source();
    });
  });
  do_test_pending();
}

function unsolicitedPauseListener(aEvent, aPacket, aContinue) {
  gContinue = aContinue;
}

function test_source()
{
  DebuggerServer.LONG_STRING_LENGTH = 200;

  gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket) {
    gThreadClient.getScripts(function (aResponse) {
      do_check_true(!!aResponse);
      do_check_true(!!aResponse.scripts);

      let script = aResponse.scripts.filter(function (s) {
        return s.url.match(/test_source-01.js$/);
      })[0];

      do_check_true(!!script);
      do_check_true(!!script.source);

      let sourceClient = gThreadClient.source(script.source);
      sourceClient.source(function (aResponse) {
        do_check_true(!!aResponse);
        do_check_true(!aResponse.error);
        do_check_true(!!aResponse.source);

        let source = aResponse.source;
        do_check_eq(source.type, "longString");

        let sourceStringClient = gThreadClient.threadLongString(source);
        let len = sourceStringClient.length;
        sourceStringClient.substring(0, len, function (aResponse) {
          do_check_true(!!aResponse);
          do_check_true(!!aResponse.substring);

          let f = do_get_file("test_source-01.js", false);
          let s = Cc["@mozilla.org/network/file-input-stream;1"]
            .createInstance(Ci.nsIFileInputStream);
          s.init(f, -1, -1, false);

          do_check_eq(NetUtil.readInputStreamToString(s, s.available()),
                      aResponse.substring);

          s.close();
          gThreadClient.resume(function () {
            finishClient(gClient);
          });
        });
      });
    });
  });

  gDebuggee.eval('stopMe()');
}
