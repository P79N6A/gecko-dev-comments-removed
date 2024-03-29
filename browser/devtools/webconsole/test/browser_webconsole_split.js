




"use strict";

const TEST_URI = "data:text/html;charset=utf-8,Web Console test for splitting";

function test() {
  
  requestLongerTimeout(2);

  let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
  let {Task} = Cu.import("resource://gre/modules/Task.jsm", {});
  let Toolbox = devtools.Toolbox;
  let toolbox;

  loadTab(TEST_URI).then(testConsoleLoadOnDifferentPanel);

  function testConsoleLoadOnDifferentPanel() {
    info("About to check console loads even when non-webconsole panel is open");

    openPanel("inspector").then(() => {
      toolbox.on("webconsole-ready", () => {
        ok(true, "Webconsole has been triggered as loaded while another tool " +
                 "is active");
        testKeyboardShortcuts();
      });

      
      toolbox.toggleSplitConsole();
    });
  }

  function testKeyboardShortcuts() {
    info("About to check that panel responds to ESCAPE keyboard shortcut");

    toolbox.once("split-console", () => {
      ok(true, "Split console has been triggered via ESCAPE keypress");
      checkAllTools();
    });

    
    EventUtils.sendKey("ESCAPE", toolbox.frame.contentWindow);
  }

  function checkAllTools() {
    info("About to check split console with each panel individually.");

    Task.spawn(function*() {
      yield openAndCheckPanel("jsdebugger");
      yield openAndCheckPanel("inspector");
      yield openAndCheckPanel("styleeditor");
      yield openAndCheckPanel("performance");
      yield openAndCheckPanel("netmonitor");

      yield checkWebconsolePanelOpened();
      testBottomHost();
    });
  }

  function getCurrentUIState() {
    let win = toolbox.doc.defaultView;
    let deck = toolbox.doc.querySelector("#toolbox-deck");
    let webconsolePanel = toolbox.webconsolePanel;
    let splitter = toolbox.doc.querySelector("#toolbox-console-splitter");

    let containerHeight = parseFloat(win.getComputedStyle(deck.parentNode)
      .getPropertyValue("height"));
    let deckHeight = parseFloat(win.getComputedStyle(deck)
      .getPropertyValue("height"));
    let webconsoleHeight = parseFloat(win.getComputedStyle(webconsolePanel)
      .getPropertyValue("height"));
    let splitterVisibility = !splitter.getAttribute("hidden");
    let openedConsolePanel = toolbox.currentToolId === "webconsole";
    let cmdButton = toolbox.doc.querySelector("#command-button-splitconsole");

    return {
      deckHeight: deckHeight,
      containerHeight: containerHeight,
      webconsoleHeight: webconsoleHeight,
      splitterVisibility: splitterVisibility,
      openedConsolePanel: openedConsolePanel,
      buttonSelected: cmdButton.hasAttribute("checked")
    };
  }

  function checkWebconsolePanelOpened() {
    info("About to check special cases when webconsole panel is open.");

    let deferred = promise.defer();

    
    toolbox.toggleSplitConsole();

    let currentUIState = getCurrentUIState();

    ok(currentUIState.splitterVisibility,
       "Splitter is visible when console is split");
    ok(currentUIState.deckHeight > 0,
       "Deck has a height > 0 when console is split");
    ok(currentUIState.webconsoleHeight > 0,
       "Web console has a height > 0 when console is split");
    ok(!currentUIState.openedConsolePanel,
       "The console panel is not the current tool");
    ok(currentUIState.buttonSelected, "The command button is selected");

    openPanel("webconsole").then(() => {
      currentUIState = getCurrentUIState();

      ok(!currentUIState.splitterVisibility,
         "Splitter is hidden when console is opened.");
      is(currentUIState.deckHeight, 0,
         "Deck has a height == 0 when console is opened.");
      is(currentUIState.webconsoleHeight, currentUIState.containerHeight,
         "Web console is full height.");
      ok(currentUIState.openedConsolePanel,
         "The console panel is the current tool");
      ok(currentUIState.buttonSelected,
         "The command button is still selected.");

      
      toolbox.toggleSplitConsole();

      currentUIState = getCurrentUIState();

      ok(!currentUIState.splitterVisibility,
         "Splitter is hidden when console is opened.");
      is(currentUIState.deckHeight, 0,
         "Deck has a height == 0 when console is opened.");
      is(currentUIState.webconsoleHeight, currentUIState.containerHeight,
         "Web console is full height.");
      ok(currentUIState.openedConsolePanel,
         "The console panel is the current tool");
      ok(currentUIState.buttonSelected,
         "The command button is still selected.");

      
      openPanel("inspector").then(() => {
        currentUIState = getCurrentUIState();
        ok(currentUIState.splitterVisibility,
           "Splitter is visible when console is split");
        ok(currentUIState.deckHeight > 0,
           "Deck has a height > 0 when console is split");
        ok(currentUIState.webconsoleHeight > 0,
           "Web console has a height > 0 when console is split");
        ok(!currentUIState.openedConsolePanel,
           "The console panel is not the current tool");
        ok(currentUIState.buttonSelected,
           "The command button is still selected.");

        toolbox.toggleSplitConsole();
        deferred.resolve();
      });
    });
    return deferred.promise;
  }

  function openPanel(toolId) {
    let deferred = promise.defer();
    let target = TargetFactory.forTab(gBrowser.selectedTab);
    gDevTools.showToolbox(target, toolId).then(function(box) {
      toolbox = box;
      deferred.resolve();
    }).then(null, console.error);
    return deferred.promise;
  }

  function openAndCheckPanel(toolId) {
    let deferred = promise.defer();
    openPanel(toolId).then(() => {
      info("Checking toolbox for " + toolId);
      checkToolboxUI(toolbox.getCurrentPanel());
      deferred.resolve();
    });
    return deferred.promise;
  }

  function checkToolboxUI() {
    let currentUIState = getCurrentUIState();

    ok(!currentUIState.splitterVisibility, "Splitter is hidden by default");
    is(currentUIState.deckHeight, currentUIState.containerHeight,
       "Deck has a height > 0 by default");
    is(currentUIState.webconsoleHeight, 0,
       "Web console is collapsed by default");
    ok(!currentUIState.openedConsolePanel,
       "The console panel is not the current tool");
    ok(!currentUIState.buttonSelected, "The command button is not selected.");

    toolbox.toggleSplitConsole();

    currentUIState = getCurrentUIState();

    ok(currentUIState.splitterVisibility,
       "Splitter is visible when console is split");
    ok(currentUIState.deckHeight > 0,
       "Deck has a height > 0 when console is split");
    ok(currentUIState.webconsoleHeight > 0,
       "Web console has a height > 0 when console is split");
    is(Math.round(currentUIState.deckHeight + currentUIState.webconsoleHeight),
       currentUIState.containerHeight,
       "Everything adds up to container height");
    ok(!currentUIState.openedConsolePanel,
       "The console panel is not the current tool");
    ok(currentUIState.buttonSelected, "The command button is selected.");

    toolbox.toggleSplitConsole();

    currentUIState = getCurrentUIState();

    ok(!currentUIState.splitterVisibility, "Splitter is hidden after toggling");
    is(currentUIState.deckHeight, currentUIState.containerHeight,
       "Deck has a height > 0 after toggling");
    is(currentUIState.webconsoleHeight, 0,
       "Web console is collapsed after toggling");
    ok(!currentUIState.openedConsolePanel,
       "The console panel is not the current tool");
    ok(!currentUIState.buttonSelected, "The command button is not selected.");
  }

  function testBottomHost() {
    checkHostType(Toolbox.HostType.BOTTOM);

    checkToolboxUI();

    toolbox.switchHost(Toolbox.HostType.SIDE).then(testSidebarHost);
  }

  function testSidebarHost() {
    checkHostType(Toolbox.HostType.SIDE);

    checkToolboxUI();

    toolbox.switchHost(Toolbox.HostType.WINDOW).then(testWindowHost);
  }

  function testWindowHost() {
    checkHostType(Toolbox.HostType.WINDOW);

    checkToolboxUI();

    toolbox.switchHost(Toolbox.HostType.BOTTOM).then(testDestroy);
  }

  function checkHostType(hostType) {
    is(toolbox.hostType, hostType, "host type is " + hostType);

    let pref = Services.prefs.getCharPref("devtools.toolbox.host");
    is(pref, hostType, "host pref is " + hostType);
  }

  function testDestroy() {
    toolbox.destroy().then(function() {
      let target = TargetFactory.forTab(gBrowser.selectedTab);
      gDevTools.showToolbox(target).then(finish);
    });
  }

  function finish() {
    toolbox = null;
    finishTest();
  }
}
