



Cu.import("resource://gre/modules/Services.jsm");
let temp = {}
Cu.import("resource:///modules/devtools/gDevTools.jsm", temp);
let DevTools = temp.DevTools;

Cu.import("resource://gre/modules/devtools/Loader.jsm", temp);
let devtools = temp.devtools;

let Toolbox = devtools.Toolbox;

let toolbox, target, tab1, tab2;

function test() {
  waitForExplicitFinish();

  gBrowser.selectedTab = tab1 = gBrowser.addTab();
  tab2 = gBrowser.addTab();
  target = TargetFactory.forTab(gBrowser.selectedTab);

  gBrowser.selectedBrowser.addEventListener("load", function onLoad(evt) {
    gBrowser.selectedBrowser.removeEventListener(evt.type, onLoad, true);
    gDevTools.showToolbox(target)
             .then(testBottomHost, console.error)
             .then(null, console.error);
  }, true);

  content.location = "data:text/html,test for opening toolbox in different hosts";
}

function testBottomHost(aToolbox) {
  toolbox = aToolbox;

  
  gBrowser.selectedTab = tab2;
  executeSoon(function() {
    is(gBrowser.selectedTab, tab2, "Correct tab is selected before calling raise");
    toolbox.raise();
    executeSoon(function() {
      is(gBrowser.selectedTab, tab1, "Correct tab was selected after calling raise");

      toolbox.switchHost(Toolbox.HostType.WINDOW).then(testWindowHost).then(null, console.error);
    });
  });
}

function testWindowHost() {
  
  window.addEventListener("focus", onFocus, true);

  
  
  let onToolboxFocus = () => {
    toolbox._host._window.removeEventListener("focus", onToolboxFocus, true);
    info("focusing main window.");
    window.focus()
  };
  
  toolbox._host._window.addEventListener("focus", onToolboxFocus, true);
}

function onFocus() {
  info("Main window is focused before calling toolbox.raise()")
  window.removeEventListener("focus", onFocus, true);

  
  let onToolboxFocusAgain = () => {
    toolbox._host._window.removeEventListener("focus", onToolboxFocusAgain, false);
    ok(true, "Toolbox window is the focused window after calling toolbox.raise()");
    cleanup();
  };
  toolbox._host._window.addEventListener("focus", onToolboxFocusAgain, false);

  
  toolbox.raise();
}

function cleanup() {
  Services.prefs.setCharPref("devtools.toolbox.host", Toolbox.HostType.BOTTOM);

  toolbox.destroy().then(function() {
    DevTools = Toolbox = toolbox = target = null;
    gBrowser.removeCurrentTab();
    gBrowser.removeCurrentTab();
    finish();
  });
}
