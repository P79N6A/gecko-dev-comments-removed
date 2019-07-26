






var gDebuggee;
var gClient;
var gThreadClient;

Components.utils.import("resource:///modules/devtools/SourceMap.jsm");

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-source-map");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestGlobalClientAndResume(gClient, "test-source-map", function(aResponse, aThreadClient) {
      gThreadClient = aThreadClient;
      test_simple_source_map();
    });
  });
  do_test_pending();
}

function test_simple_source_map()
{
  
  
  let expectedSources = new Set(["http://example.com/www/js/a.js",
                                 "http://example.com/www/js/b.js",
                                 "http://example.com/www/js/c.js"]);

  let numNewSources = 0;

  gClient.addListener("newSource", function _onNewSource(aEvent, aPacket) {
    if (++numNewSources !== 3) {
      return;
    }
    gClient.removeListener("newSource", _onNewSource);

    gThreadClient.getSources(function (aResponse) {
      do_check_true(!aResponse.error, "Should not get an error");

      for (let s of aResponse.sources) {
        do_check_true(expectedSources.has(s.url),
                      "The source's url should be one of our original sources");
        expectedSources.delete(s.url);
      }

      do_check_eq(expectedSources.size, 0,
                  "Shouldn't be expecting any more sources");

      finishClient(gClient);
    });
  });

  let { code, map } = (new SourceNode(null, null, null, [
    new SourceNode(1, 0, "a.js", "function a() { return 'a'; }\n"),
    new SourceNode(1, 0, "b.js", "function b() { return 'b'; }\n"),
    new SourceNode(1, 0, "c.js", "function c() { return 'c'; }\n"),
  ])).toStringWithSourceMap({
    file: "abc.js",
    sourceRoot: "http://example.com/www/js/"
  });

  code += "//@ sourceMappingURL=data:text/json;base64," + btoa(map.toString());

  Components.utils.evalInSandbox(code, gDebuggee, "1.8",
                                 "http://example.com/www/js/abc.js", 1);
}
