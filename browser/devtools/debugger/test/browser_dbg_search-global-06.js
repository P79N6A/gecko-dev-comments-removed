







const TAB_URL = EXAMPLE_URL + "doc_script-switching-01.html";

let gTab, gPanel, gDebugger;
let gEditor, gSources, gSearchView, gSearchBox;

function test() {
  initDebugger(TAB_URL).then(([aTab,, aPanel]) => {
    gTab = aTab;
    gPanel = aPanel;
    gDebugger = gPanel.panelWin;
    gEditor = gDebugger.DebuggerView.editor;
    gSources = gDebugger.DebuggerView.Sources;
    gSearchView = gDebugger.DebuggerView.GlobalSearch;
    gSearchBox = gDebugger.DebuggerView.Filtering._searchbox;

    waitForSourceAndCaretAndScopes(gPanel, "-02.js", 1)
      .then(doSearch)
      .then(testFocusLost)
      .then(doSearch)
      .then(testEscape)
      .then(() => resumeDebuggerThenCloseAndFinish(gPanel))
      .then(null, aError => {
        ok(false, "Got an error: " + aError.message + "\n" + aError.stack);
      });

    callInTab(gTab, "firstCall");
  });
}

function doSearch() {
  let deferred = promise.defer();

  is(gSearchView.itemCount, 0,
    "The global search pane shouldn't have any entries yet.");
  is(gSearchView.widget._parent.hidden, true,
    "The global search pane shouldn't be visible yet.");
  is(gSearchView._splitter.hidden, true,
    "The global search pane splitter shouldn't be visible yet.");

  gDebugger.once(gDebugger.EVENTS.GLOBAL_SEARCH_MATCH_FOUND, () => {
    
    
    executeSoon(() => {
      info("Current source url:\n" + getSelectedSourceURL(gSources));
      info("Debugger editor text:\n" + gEditor.getText());

      ok(isCaretPos(gPanel, 6),
        "The editor shouldn't have jumped to a matching line yet.");
      ok(getSelectedSourceURL(gSources).includes("-02.js"),
        "The current source shouldn't have changed after a global search.");
      is(gSources.visibleItems.length, 2,
        "Not all the sources are shown after the global search.");

      deferred.resolve();
    });
  });

  setText(gSearchBox, "!a");

  return deferred.promise;
}

function testFocusLost() {
  is(gSearchView.itemCount, 2,
    "The global search pane should have some entries from the previous search.");
  is(gSearchView.widget._parent.hidden, false,
    "The global search pane should be visible from the previous search.");
  is(gSearchView._splitter.hidden, false,
    "The global search pane splitter should be visible from the previous search.");

  gDebugger.DebuggerView.editor.focus();

  info("Current source url:\n" + getSelectedSourceURL(gSources));
  info("Debugger editor text:\n" + gEditor.getText());

  is(gSearchView.itemCount, 0,
    "The global search pane shouldn't have any child nodes after clearing.");
  is(gSearchView.widget._parent.hidden, true,
    "The global search pane shouldn't be visible after clearing.");
  is(gSearchView._splitter.hidden, true,
    "The global search pane splitter shouldn't be visible after clearing.");
}

function testEscape() {
  is(gSearchView.itemCount, 2,
    "The global search pane should have some entries from the previous search.");
  is(gSearchView.widget._parent.hidden, false,
    "The global search pane should be visible from the previous search.");
  is(gSearchView._splitter.hidden, false,
    "The global search pane splitter should be visible from the previous search.");

   gSearchBox.focus();
   EventUtils.sendKey("ESCAPE", gDebugger);

  is(gSearchView.itemCount, 0,
    "The global search pane shouldn't have any child nodes after clearing.");
  is(gSearchView.widget._parent.hidden, true,
    "The global search pane shouldn't be visible after clearing.");
  is(gSearchView._splitter.hidden, true,
    "The global search pane splitter shouldn't be visible after clearing.");
}

registerCleanupFunction(function() {
  gTab = null;
  gPanel = null;
  gDebugger = null;
  gEditor = null;
  gSources = null;
  gSearchView = null;
  gSearchBox = null;
});
