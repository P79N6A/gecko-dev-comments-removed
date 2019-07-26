






let gTab, gDebuggee, gPanel, gClient, gThreadClient;

const TAB_URL = EXAMPLE_URL + "doc_pretty-print-2.html";

function test() {
  initDebugger(TAB_URL).then(([aTab, aDebuggee, aPanel]) => {
    gTab = aTab;
    gDebuggee = aDebuggee;
    gPanel = aPanel;
    gClient = gPanel.panelWin.gClient;
    gThreadClient = gPanel.panelWin.DebuggerController.activeThread;

    findSource();
  });
}

function findSource() {
  gThreadClient.getSources(({ error, sources }) => {
    ok(!error);
    sources = sources.filter(s => s.url.contains('code_ugly-2.js'));
    is(sources.length, 1);
    prettyPrintSource(sources[0]);
  });
}

function prettyPrintSource(source) {
  gThreadClient.source(source).prettyPrint(4, testPrettyPrinted);
}

function testPrettyPrinted({ error, source}) {
  ok(!error);
  ok(source.contains("\n    "));

  closeDebuggerAndFinish(gPanel);
}

registerCleanupFunction(function() {
  gTab = gDebuggee = gPanel = gClient = gThreadClient = null;
});
