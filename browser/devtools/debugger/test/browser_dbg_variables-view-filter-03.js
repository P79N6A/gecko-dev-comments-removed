







const TAB_URL = EXAMPLE_URL + "doc_with-frame.html";

let gTab, gPanel, gDebugger;
let gVariables, gSearchBox;

function test() {
  
  requestLongerTimeout(2);

  initDebugger(TAB_URL).then(([aTab,, aPanel]) => {
    gTab = aTab;
    gPanel = aPanel;
    gDebugger = gPanel.panelWin;
    gVariables = gDebugger.DebuggerView.Variables;
    gSearchBox = gDebugger.DebuggerView.Filtering._searchbox;

    
    
    promise.all([
      waitForSourceAndCaret(gPanel, ".html", 22),
      waitForDebuggerEvents(gPanel, gDebugger.EVENTS.FETCHED_SCOPES),
      waitForDebuggerEvents(gPanel, gDebugger.EVENTS.FETCHED_VARIABLES)
    ]).then(prepareVariablesAndProperties)
      .then(testVariablesAndPropertiesFiltering)
      .then(() => resumeDebuggerThenCloseAndFinish(gPanel))
      .then(null, aError => {
        ok(false, "Got an error: " + aError.message + "\n" + aError.stack);
      });

    sendMouseClickToTab(gTab, content.document.querySelector("button"));
  });
}

function testVariablesAndPropertiesFiltering() {
  let localScope = gVariables.getScopeAtIndex(0);
  let withScope = gVariables.getScopeAtIndex(1);
  let functionScope = gVariables.getScopeAtIndex(2);
  let globalScope = gVariables.getScopeAtIndex(3);

  function testFiltered() {
    is(localScope.expanded, true,
      "The localScope should be expanded.");
    is(withScope.expanded, true,
      "The withScope should be expanded.");
    is(functionScope.expanded, true,
      "The functionScope should be expanded.");
    is(globalScope.expanded, true,
      "The globalScope should be expanded.");

    is(localScope.target.querySelectorAll(".variables-view-variable:not([unmatched])").length, 1,
      "There should be 1 variable displayed in the local scope.");
    is(withScope.target.querySelectorAll(".variables-view-variable:not([unmatched])").length, 0,
      "There should be 0 variables displayed in the with scope.");
    is(functionScope.target.querySelectorAll(".variables-view-variable:not([unmatched])").length, 0,
      "There should be 0 variables displayed in the function scope.");
    is(globalScope.target.querySelectorAll(".variables-view-variable:not([unmatched])").length, 0,
      "There should be 0 variables displayed in the global scope.");

    is(localScope.target.querySelectorAll(".variables-view-property:not([unmatched])").length, 0,
      "There should be 0 properties displayed in the local scope.");
    is(withScope.target.querySelectorAll(".variables-view-property:not([unmatched])").length, 0,
      "There should be 0 properties displayed in the with scope.");
    is(functionScope.target.querySelectorAll(".variables-view-property:not([unmatched])").length, 0,
      "There should be 0 properties displayed in the function scope.");
    is(globalScope.target.querySelectorAll(".variables-view-property:not([unmatched])").length, 0,
      "There should be 0 properties displayed in the global scope.");
  }

  function firstFilter() {
    typeText(gSearchBox, "*alpha");
    testFiltered("alpha");

    is(localScope.target.querySelectorAll(".variables-view-variable:not([unmatched]) > .title > .name")[0].getAttribute("value"),
      "alpha", "The only inner variable displayed should be 'alpha'");
  }

  function secondFilter() {
    localScope.collapse();
    withScope.collapse();
    functionScope.collapse();
    globalScope.collapse();

    is(localScope.expanded, false,
      "The localScope should not be expanded.");
    is(withScope.expanded, false,
      "The withScope should not be expanded.");
    is(functionScope.expanded, false,
      "The functionScope should not be expanded.");
    is(globalScope.expanded, false,
      "The globalScope should not be expanded.");

    backspaceText(gSearchBox, 6);
    typeText(gSearchBox, "*beta");
    testFiltered("beta");

    is(localScope.target.querySelectorAll(".variables-view-variable:not([unmatched]) > .title > .name")[0].getAttribute("value"),
      "beta", "The only inner variable displayed should be 'beta'");
  }

  firstFilter();
  secondFilter();
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

    deferred.resolve();
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
  gVariables = null;
  gSearchBox = null;
});
