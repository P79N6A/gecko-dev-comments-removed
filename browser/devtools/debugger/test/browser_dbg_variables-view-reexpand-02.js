







const TAB_URL = EXAMPLE_URL + "doc_with-frame.html";

let gTab, gPanel, gDebugger;
let gBreakpoints, gSources, gVariables;

function test() {
  
  requestLongerTimeout(4);

  initDebugger(TAB_URL).then(([aTab,, aPanel]) => {
    gTab = aTab;
    gPanel = aPanel;
    gDebugger = gPanel.panelWin;
    gBreakpoints = gDebugger.DebuggerController.Breakpoints;
    gSources = gDebugger.DebuggerView.Sources;
    gVariables = gDebugger.DebuggerView.Variables;

    
    gVariables.commitHierarchyIgnoredItems = Object.create(null);

    waitForSourceShown(gPanel, ".html")
      .then(addBreakpoint)
      .then(() => ensureThreadClientState(gPanel, "resumed"))
      .then(pauseDebuggee)
      .then(prepareVariablesAndProperties)
      .then(stepInDebuggee)
      .then(testVariablesExpand)
      .then(() => resumeDebuggerThenCloseAndFinish(gPanel))
      .then(null, aError => {
        ok(false, "Got an error: " + aError.message + "\n" + aError.stack);
      });
  });
}

function addBreakpoint() {
  return gBreakpoints.addBreakpoint({ actor: gSources.selectedValue, line: 21 });
}

function pauseDebuggee() {
  generateMouseClickInTab(gTab, "content.document.querySelector('button')");

  
  
  return promise.all([
    waitForDebuggerEvents(gPanel, gDebugger.EVENTS.FETCHED_SCOPES),
    waitForDebuggerEvents(gPanel, gDebugger.EVENTS.FETCHED_VARIABLES)
  ]);
}

function stepInDebuggee() {
  
  
  executeSoon(() => {
    EventUtils.sendMouseEvent({ type: "mousedown" },
      gDebugger.document.querySelector("#step-in"),
      gDebugger);
  });

  return promise.all([
    waitForDebuggerEvents(gPanel, gDebugger.EVENTS.FETCHED_SCOPES, 1),
    waitForDebuggerEvents(gPanel, gDebugger.EVENTS.FETCHED_VARIABLES, 3),
    waitForDebuggerEvents(gPanel, gDebugger.EVENTS.FETCHED_PROPERTIES, 4),
  ]);
}

function testVariablesExpand() {
  let localScope = gVariables.getScopeAtIndex(0);
  let withScope = gVariables.getScopeAtIndex(1);
  let functionScope = gVariables.getScopeAtIndex(2);
  let globalScope = gVariables.getScopeAtIndex(3);

  let thisVar = localScope.get("this");
  let windowVar = thisVar.get("window");
  let documentVar = windowVar.get("document");
  let locationVar = documentVar.get("location");

  is(localScope.target.querySelector(".arrow").hasAttribute("open"), true,
    "The localScope arrow should still be expanded.");
  is(withScope.target.querySelector(".arrow").hasAttribute("open"), true,
    "The withScope arrow should still be expanded.");
  is(functionScope.target.querySelector(".arrow").hasAttribute("open"), true,
    "The functionScope arrow should still be expanded.");
  is(globalScope.target.querySelector(".arrow").hasAttribute("open"), true,
    "The globalScope arrow should still be expanded.");
  is(thisVar.target.querySelector(".arrow").hasAttribute("open"), true,
    "The thisVar arrow should still be expanded.");
  is(windowVar.target.querySelector(".arrow").hasAttribute("open"), true,
    "The windowVar arrow should still be expanded.");
  is(documentVar.target.querySelector(".arrow").hasAttribute("open"), true,
    "The documentVar arrow should still be expanded.");
  is(locationVar.target.querySelector(".arrow").hasAttribute("open"), true,
    "The locationVar arrow should still be expanded.");

  is(localScope.target.querySelector(".variables-view-element-details").hasAttribute("open"), true,
    "The localScope enumerables should still be expanded.");
  is(withScope.target.querySelector(".variables-view-element-details").hasAttribute("open"), true,
    "The withScope enumerables should still be expanded.");
  is(functionScope.target.querySelector(".variables-view-element-details").hasAttribute("open"), true,
    "The functionScope enumerables should still be expanded.");
  is(globalScope.target.querySelector(".variables-view-element-details").hasAttribute("open"), true,
    "The globalScope enumerables should still be expanded.");
  is(thisVar.target.querySelector(".variables-view-element-details").hasAttribute("open"), true,
    "The thisVar enumerables should still be expanded.");
  is(windowVar.target.querySelector(".variables-view-element-details").hasAttribute("open"), true,
    "The windowVar enumerables should still be expanded.");
  is(documentVar.target.querySelector(".variables-view-element-details").hasAttribute("open"), true,
    "The documentVar enumerables should still be expanded.");
  is(locationVar.target.querySelector(".variables-view-element-details").hasAttribute("open"), true,
    "The locationVar enumerables should still be expanded.");

  is(localScope.expanded, true,
    "The localScope expanded getter should return true.");
  is(withScope.expanded, true,
    "The withScope expanded getter should return true.");
  is(functionScope.expanded, true,
    "The functionScope expanded getter should return true.");
  is(globalScope.expanded, true,
    "The globalScope expanded getter should return true.");
  is(thisVar.expanded, true,
    "The thisVar expanded getter should return true.");
  is(windowVar.expanded, true,
    "The windowVar expanded getter should return true.");
  is(documentVar.expanded, true,
    "The documentVar expanded getter should return true.");
  is(locationVar.expanded, true,
    "The locationVar expanded getter should return true.");
}

function prepareVariablesAndProperties() {
  let deferred = promise.defer();

  let localScope = gVariables.getScopeAtIndex(0);
  let withScope = gVariables.getScopeAtIndex(1);
  let functionScope = gVariables.getScopeAtIndex(2);
  let globalScope = gVariables.getScopeAtIndex(3);

  is(localScope.expanded, true,
    "The localScope should be expanded.");
  is(withScope.expanded, false,
    "The withScope should not be expanded yet.");
  is(functionScope.expanded, false,
    "The functionScope should not be expanded yet.");
  is(globalScope.expanded, false,
    "The globalScope should not be expanded yet.");

  
  
  waitForDebuggerEvents(gPanel, gDebugger.EVENTS.FETCHED_VARIABLES, 2).then(() => {
    is(localScope.expanded, true,
      "The localScope should now be expanded.");
    is(withScope.expanded, true,
      "The withScope should now be expanded.");
    is(functionScope.expanded, true,
      "The functionScope should now be expanded.");
    is(globalScope.expanded, true,
      "The globalScope should now be expanded.");

    let thisVar = localScope.get("this");

    waitForDebuggerEvents(gPanel, gDebugger.EVENTS.FETCHED_PROPERTIES, 1).then(() => {
      let windowVar = thisVar.get("window");

      waitForDebuggerEvents(gPanel, gDebugger.EVENTS.FETCHED_PROPERTIES, 1).then(() => {
        let documentVar = windowVar.get("document");

        waitForDebuggerEvents(gPanel, gDebugger.EVENTS.FETCHED_PROPERTIES, 1).then(() => {
          let locationVar = documentVar.get("location");

          waitForDebuggerEvents(gPanel, gDebugger.EVENTS.FETCHED_PROPERTIES, 1).then(() => {
            is(thisVar.expanded, true,
              "The local scope 'this' should be expanded.");
            is(windowVar.expanded, true,
              "The local scope 'this.window' should be expanded.");
            is(documentVar.expanded, true,
              "The local scope 'this.window.document' should be expanded.");
            is(locationVar.expanded, true,
              "The local scope 'this.window.document.location' should be expanded.");

            deferred.resolve();
          });

          locationVar.expand();
        });

        documentVar.expand();
      });

      windowVar.expand();
    });

    thisVar.expand();
  });

  withScope.expand();
  functionScope.expand();
  globalScope.expand();

  return deferred.promise;
}

registerCleanupFunction(function() {
  gTab = null;
  gPanel = null;
  gDebugger = null;
  gBreakpoints = null;
  gSources = null;
  gVariables = null;
});
