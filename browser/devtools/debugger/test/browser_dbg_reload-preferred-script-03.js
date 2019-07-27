







const TAB_URL = EXAMPLE_URL + "doc_script-switching-01.html";
const FIRST_URL = EXAMPLE_URL + "code_script-switching-01.js";
const SECOND_URL = EXAMPLE_URL + "code_script-switching-02.js";

let gTab, gPanel, gDebugger;
let gSources;

function test() {
  initDebugger(TAB_URL).then(([aTab,, aPanel]) => {
    gTab = aTab;
    gPanel = aPanel;
    gDebugger = gPanel.panelWin;
    gSources = gDebugger.DebuggerView.Sources;

    waitForSourceShown(gPanel, FIRST_URL)
      .then(() => testSource("", FIRST_URL))
      .then(() => switchToSource(SECOND_URL))
      .then(() => testSource(SECOND_URL))
      .then(() => switchToSource(FIRST_URL))
      .then(() => testSource(FIRST_URL))
      .then(() => closeDebuggerAndFinish(gPanel))
      .then(null, aError => {
        ok(false, "Got an error: " + aError.message + "\n" + aError.stack);
      });
  });
}

function testSource(aPreferredUrl, aSelectedUrl = aPreferredUrl) {
  info("Currently preferred source: " + gSources.preferredValue);
  info("Currently selected source: " + gSources.selectedValue);

  is(gSources.preferredValue, aPreferredUrl,
    "The preferred source url wasn't set correctly.");
  is(gSources.selectedValue, aSelectedUrl,
    "The selected source isn't the correct one.");
}

function switchToSource(aUrl) {
  let finished = waitForSourceShown(gPanel, aUrl);
  gSources.preferredSource = aUrl;
  return finished;
}

registerCleanupFunction(function() {
  gTab = null;
  gPanel = null;
  gDebugger = null;
  gSources = null;
});
