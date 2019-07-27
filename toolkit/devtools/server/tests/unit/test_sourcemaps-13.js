






var gDebuggee;
var gClient;
var gThreadClient;
var gTabClient;

Components.utils.import("resource:///modules/devtools/SourceMap.jsm");

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-source-map");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestTabAndResume(gClient, "test-source-map", function(aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      gTabClient = aTabClient;
      setup_code();
    });
  });
  do_test_pending();
}




const MAP_FILE_NAME = "temporary-generated.txt";

const TEMP_FILE_1 = "temporary1.js";
const TEMP_FILE_2 = "temporary2.js";
const TEMP_GENERATED_SOURCE = "temporary-generated.js";

function setup_code() {
  let node = new SourceNode(1, 0,
                            getFileUrl(TEMP_FILE_1, true),
                            "function temporary1() {}\n");
  let { code, map } = node.toStringWithSourceMap({
    file: getFileUrl(TEMP_GENERATED_SOURCE, true)
  });

  code += "//# sourceMappingURL=" + getFileUrl(MAP_FILE_NAME, true);
  writeFile(MAP_FILE_NAME, map.toString());

  Cu.evalInSandbox(code,
                   gDebuggee,
                   "1.8",
                   getFileUrl(TEMP_GENERATED_SOURCE, true),
                   1);

  test_initial_sources();
}

function test_initial_sources() {
  gThreadClient.getSources(function ({ error, sources }) {
    do_check_true(!error);
    sources = sources.filter(source => source.url);
    do_check_eq(sources.length, 1);
    do_check_eq(sources[0].url, getFileUrl(TEMP_FILE_1, true));
    reload(gTabClient).then(setup_new_code);
  });
}

function setup_new_code() {
  let node = new SourceNode(1, 0,
                            getFileUrl(TEMP_FILE_2, true),
                            "function temporary2() {}\n");
  let { code, map } = node.toStringWithSourceMap({
    file: getFileUrl(TEMP_GENERATED_SOURCE, true)
  });

  code += "\n//# sourceMappingURL=" + getFileUrl(MAP_FILE_NAME, true);
  writeFile(MAP_FILE_NAME, map.toString());

  gThreadClient.addOneTimeListener("newSource", test_new_sources);
  Cu.evalInSandbox(code,
                   gDebuggee,
                   "1.8",
                   getFileUrl(TEMP_GENERATED_SOURCE, true),
                   1);
}

function test_new_sources() {
  gThreadClient.getSources(function ({ error, sources }) {
    do_check_true(!error);
    sources = sources.filter(source => source.url);

    
    do_check_eq(sources.length, 1);
    let s = sources.filter(s => s.url === getFileUrl(TEMP_FILE_2, true))[0];
    do_check_true(!!s);

    finish_test();
  });
}

function finish_test() {
  do_get_file(MAP_FILE_NAME).remove(false);
  finishClient(gClient);
}
