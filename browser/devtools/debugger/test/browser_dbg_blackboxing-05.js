







const TAB_URL = EXAMPLE_URL + "doc_binary_search.html";

let gTab, gPanel, gDebugger;
let gDeck;

function test() {
  initDebugger(TAB_URL).then(([aTab,, aPanel]) => {
    gTab = aTab;
    gPanel = aPanel;
    gDebugger = gPanel.panelWin;
    gDeck = gDebugger.document.getElementById("editor-deck");

    waitForSourceShown(gPanel, ".coffee")
      .then(testSourceEditorShown)
      .then(toggleBlackBoxing.bind(null, gPanel))
      .then(testBlackBoxMessageShown)
      .then(clickStopBlackBoxingButton)
      .then(testSourceEditorShownAgain)
      .then(() => closeDebuggerAndFinish(gPanel))
      .then(null, aError => {
        ok(false, "Got an error: " + aError.message + "\n" + aError.stack);
      });
  });
}

function testSourceEditorShown() {
  is(gDeck.selectedIndex, "0",
    "The first item in the deck should be selected (the source editor).");
}

function testBlackBoxMessageShown() {
  is(gDeck.selectedIndex, "1",
    "The second item in the deck should be selected (the black box message).");
}

function clickStopBlackBoxingButton() {
  
  executeSoon(() => getEditorBlackboxMessageButton().click());
  return waitForThreadEvents(gPanel, "blackboxchange");
}

function testSourceEditorShownAgain() {
  
  
  return new Promise(resolve => {
    is(gDeck.selectedIndex, "0",
      "The first item in the deck should be selected again (the source editor).");
    resolve();
  });
}

function getEditorBlackboxMessageButton() {
  return gDebugger.document.getElementById("black-boxed-message-button");
}

registerCleanupFunction(function() {
  gTab = null;
  gPanel = null;
  gDebugger = null;
  gDeck = null;
});
