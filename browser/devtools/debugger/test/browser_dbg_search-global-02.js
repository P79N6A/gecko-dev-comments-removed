







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
      .then(firstSearch)
      .then(doFirstJump)
      .then(doSecondJump)
      .then(doWrapAroundJump)
      .then(doBackwardsWrapAroundJump)
      .then(testSearchTokenEmpty)
      .then(() => resumeDebuggerThenCloseAndFinish(gPanel))
      .then(null, aError => {
        ok(false, "Got an error: " + aError.message + "\n" + aError.stack);
      });

    callInTab(gTab, "firstCall");
  });
}

function firstSearch() {
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

  setText(gSearchBox, "!function");

  return deferred.promise;
}

function doFirstJump() {
  let deferred = promise.defer();

  waitForSourceAndCaret(gPanel, "-01.js", 4).then(() => {
    info("Current source url:\n" + getSelectedSourceURL(gSources));
    info("Debugger editor text:\n" + gEditor.getText());

    ok(getSelectedSourceURL(gSources).includes("-01.js"),
      "The currently shown source is incorrect (1).");
    is(gSources.visibleItems.length, 2,
      "Not all the sources are shown after the global search (1).");

    
    executeSoon(() => {
      ok(isCaretPos(gPanel, 4, 9),
        "The editor didn't jump to the correct line (1).");
      is(gEditor.getSelection(), "function",
        "The editor didn't select the correct text (1).");

      deferred.resolve();
    });
  });

  EventUtils.sendKey("DOWN", gDebugger);

  return deferred.promise;
}

function doSecondJump() {
  let deferred = promise.defer();

  waitForSourceAndCaret(gPanel, "-02.js", 4).then(() => {
    info("Current source url:\n" + getSelectedSourceURL(gSources));
    info("Debugger editor text:\n" + gEditor.getText());

    ok(getSelectedSourceURL(gSources).includes("-02.js"),
      "The currently shown source is incorrect (2).");
    is(gSources.visibleItems.length, 2,
      "Not all the sources are shown after the global search (2).");

    
    executeSoon(() => {
      ok(isCaretPos(gPanel, 4, 9),
        "The editor didn't jump to the correct line (2).");
      is(gEditor.getSelection(), "function",
        "The editor didn't select the correct text (2).");

      deferred.resolve();
    });
  });

  EventUtils.sendKey("DOWN", gDebugger);

  return deferred.promise;
}

function doWrapAroundJump() {
  let deferred = promise.defer();

  waitForSourceAndCaret(gPanel, "-01.js", 4).then(() => {
    info("Current source url:\n" + getSelectedSourceURL(gSources));
    info("Debugger editor text:\n" + gEditor.getText());

    ok(getSelectedSourceURL(gSources).includes("-01.js"),
      "The currently shown source is incorrect (3).");
    is(gSources.visibleItems.length, 2,
      "Not all the sources are shown after the global search (3).");

    
    executeSoon(() => {
      ok(isCaretPos(gPanel, 4, 9),
        "The editor didn't jump to the correct line (3).");
      is(gEditor.getSelection(), "function",
        "The editor didn't select the correct text (3).");

      deferred.resolve();
    });
  });

  EventUtils.sendKey("DOWN", gDebugger);
  EventUtils.sendKey("DOWN", gDebugger);

  return deferred.promise;
}

function doBackwardsWrapAroundJump() {
  let deferred = promise.defer();

  waitForSourceAndCaret(gPanel, "-02.js", 7).then(() => {
    info("Current source url:\n" + getSelectedSourceURL(gSources));
    info("Debugger editor text:\n" + gEditor.getText());

    ok(getSelectedSourceURL(gSources).includes("-02.js"),
      "The currently shown source is incorrect (4).");
    is(gSources.visibleItems.length, 2,
      "Not all the sources are shown after the global search (4).");

    
    executeSoon(() => {
      ok(isCaretPos(gPanel, 7, 11),
        "The editor didn't jump to the correct line (4).");
      is(gEditor.getSelection(), "function",
        "The editor didn't select the correct text (4).");

      deferred.resolve();
    });
  });

  EventUtils.sendKey("UP", gDebugger);

  return deferred.promise;
}

function testSearchTokenEmpty() {
  backspaceText(gSearchBox, 4);

  info("Current source url:\n" + getSelectedSourceURL(gSources));
  info("Debugger editor text:\n" + gEditor.getText());

  ok(getSelectedSourceURL(gSources).includes("-02.js"),
    "The currently shown source is incorrect (4).");
  is(gSources.visibleItems.length, 2,
    "Not all the sources are shown after the global search (4).");
  ok(isCaretPos(gPanel, 7, 11),
    "The editor didn't remain at the correct line (4).");
  is(gEditor.getSelection(), "",
    "The editor shouldn't keep the previous text selected (4).");

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
