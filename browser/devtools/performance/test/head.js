

"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

let { Services } = Cu.import("resource://gre/modules/Services.jsm", {});
let { Preferences } = Cu.import("resource://gre/modules/Preferences.jsm", {});
let { Task } = Cu.import("resource://gre/modules/Task.jsm", {});
let { Promise } = Cu.import("resource://gre/modules/Promise.jsm", {});
let { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
let { gDevTools } = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});
let { DevToolsUtils } = Cu.import("resource://gre/modules/devtools/DevToolsUtils.jsm", {});
let { DebuggerServer } = Cu.import("resource://gre/modules/devtools/dbg-server.jsm", {});
let { merge } = devtools.require("sdk/util/object");
let { getPerformanceActorsConnection, PerformanceFront } = devtools.require("devtools/performance/front");

let nsIProfilerModule = Cc["@mozilla.org/tools/profiler;1"].getService(Ci.nsIProfiler);
let TargetFactory = devtools.TargetFactory;
let mm = null;

const FRAME_SCRIPT_UTILS_URL = "chrome://browser/content/devtools/frame-script-utils.js"
const EXAMPLE_URL = "http://example.com/browser/browser/devtools/performance/test/";
const SIMPLE_URL = EXAMPLE_URL + "doc_simple-test.html";

const MEMORY_SAMPLE_PROB_PREF = "devtools.performance.memory.sample-probability";
const MEMORY_MAX_LOG_LEN_PREF = "devtools.performance.memory.max-log-length";
const PROFILER_BUFFER_SIZE_PREF = "devtools.performance.profiler.buffer-size";
const PROFILER_SAMPLE_RATE_PREF = "devtools.performance.profiler.sample-frequency-khz";

const FRAMERATE_PREF = "devtools.performance.ui.enable-framerate";
const MEMORY_PREF = "devtools.performance.ui.enable-memory";

const PLATFORM_DATA_PREF = "devtools.performance.ui.show-platform-data";
const IDLE_PREF = "devtools.performance.ui.show-idle-blocks";
const INVERT_PREF = "devtools.performance.ui.invert-call-tree";
const INVERT_FLAME_PREF = "devtools.performance.ui.invert-flame-graph";
const FLATTEN_PREF = "devtools.performance.ui.flatten-tree-recursion";
const JIT_PREF = "devtools.performance.ui.show-jit-optimizations";


waitForExplicitFinish();

gDevTools.testing = true;

let DEFAULT_PREFS = [
  "devtools.debugger.log",
  "devtools.performance.ui.invert-call-tree",
  "devtools.performance.ui.flatten-tree-recursion",
  "devtools.performance.ui.show-platform-data",
  "devtools.performance.ui.show-idle-blocks",
  "devtools.performance.ui.enable-memory",
  "devtools.performance.ui.enable-framerate",
  "devtools.performance.ui.show-jit-optimizations",
  "devtools.performance.memory.sample-probability",
  "devtools.performance.memory.max-log-length",
  "devtools.performance.profiler.buffer-size",
  "devtools.performance.profiler.sample-frequency-khz",
  "devtools.performance.ui.retro-mode",
].reduce((prefs, pref) => {
  prefs[pref] = Preferences.get(pref);
  return prefs;
}, {});


Services.prefs.setBoolPref("devtools.performance.enabled", true);


Services.prefs.setBoolPref("devtools.debugger.log", false);



Services.prefs.setBoolPref("devtools.performance.ui.retro-mode", false);







function loadFrameScripts () {
  mm = gBrowser.selectedBrowser.messageManager;
  mm.loadFrameScript(FRAME_SCRIPT_UTILS_URL, false);
}

registerCleanupFunction(() => {
  gDevTools.testing = false;
  info("finish() was called, cleaning up...");

  
  Object.keys(DEFAULT_PREFS).forEach(pref => {
    Preferences.set(pref, DEFAULT_PREFS[pref]);
  });

  
  nsIProfilerModule.StopProfiler();

  Cu.forceGC();
});

function addTab(aUrl, aWindow) {
  info("Adding tab: " + aUrl);

  let deferred = Promise.defer();
  let targetWindow = aWindow || window;
  let targetBrowser = targetWindow.gBrowser;

  targetWindow.focus();
  let tab = targetBrowser.selectedTab = targetBrowser.addTab(aUrl);
  let linkedBrowser = tab.linkedBrowser;

  linkedBrowser.addEventListener("load", function onLoad() {
    linkedBrowser.removeEventListener("load", onLoad, true);
    info("Tab added and finished loading: " + aUrl);
    deferred.resolve(tab);
  }, true);

  return deferred.promise;
}

function removeTab(aTab, aWindow) {
  info("Removing tab.");

  let deferred = Promise.defer();
  let targetWindow = aWindow || window;
  let targetBrowser = targetWindow.gBrowser;
  let tabContainer = targetBrowser.tabContainer;

  tabContainer.addEventListener("TabClose", function onClose(aEvent) {
    tabContainer.removeEventListener("TabClose", onClose, false);
    info("Tab removed and finished closing.");
    deferred.resolve();
  }, false);

  targetBrowser.removeTab(aTab);
  return deferred.promise;
}

function handleError(aError) {
  ok(false, "Got an error: " + aError.message + "\n" + aError.stack);
  finish();
}

function once(aTarget, aEventName, aUseCapture = false, spread = false) {
  info(`Waiting for event: '${aEventName}' on ${aTarget}`);

  let deferred = Promise.defer();

  for (let [add, remove] of [
    ["on", "off"], 
    ["addEventListener", "removeEventListener"],
    ["addListener", "removeListener"]
  ]) {
    if ((add in aTarget) && (remove in aTarget)) {
      aTarget[add](aEventName, function onEvent(...aArgs) {
        info(`Received event: '${aEventName}' on ${aTarget}`);
        aTarget[remove](aEventName, onEvent, aUseCapture);
        deferred.resolve(spread ? aArgs : aArgs[0]);
      }, aUseCapture);
      break;
    }
  }

  return deferred.promise;
}





function onceSpread(aTarget, aEventName, aUseCapture) {
  return once(aTarget, aEventName, aUseCapture, true);
}

function test () {
  Task.spawn(spawnTest).then(finish, handleError);
}

function initBackend(aUrl, targetOps={}) {
  info("Initializing a performance front.");

  if (!DebuggerServer.initialized) {
    DebuggerServer.init();
    DebuggerServer.addBrowserActors();
  }

  return Task.spawn(function*() {
    let tab = yield addTab(aUrl);
    let target = TargetFactory.forTab(tab);

    yield target.makeRemote();

    
    
    
    
    
    merge(target, targetOps);

    let connection = getPerformanceActorsConnection(target);
    yield connection.open();

    let front = new PerformanceFront(connection);
    return { target, front };
  });
}

function initPerformance(aUrl, selectedTool="performance", targetOps={}) {
  info("Initializing a performance pane.");

  return Task.spawn(function*() {
    let tab = yield addTab(aUrl);
    let target = TargetFactory.forTab(tab);

    yield target.makeRemote();

    
    
    
    
    
    merge(target, targetOps);

    let toolbox = yield gDevTools.showToolbox(target, selectedTool);
    let panel = toolbox.getCurrentPanel();
    return { target, panel, toolbox };
  });
}





function initConsole(aUrl) {
  return Task.spawn(function*() {
    let { target, toolbox, panel } = yield initPerformance(aUrl, "webconsole");
    let { hud } = panel;
    return {
      target, toolbox, panel, console: {
        profile: (s) => consoleExecute(hud, "profile", s),
        profileEnd: (s) => consoleExecute(hud, "profileEnd", s)
      }
    };
  });
}

function consoleExecute (console, method, val) {
  let { ui, jsterm } = console;
  let { promise, resolve } = Promise.defer();
  let message = `console.${method}("${val}")`;

  ui.on("new-messages", handler);
  jsterm.execute(message);

  let { console: c } = Cu.import("resource://gre/modules/devtools/Console.jsm", {});
  function handler (event, messages) {
    for (let msg of messages) {
      if (msg.response._message === message) {
        ui.off("new-messages", handler);
        resolve();
        return;
      }
    }
  }
  return promise;
}

function waitForProfilerConnection() {
  let { promise, resolve } = Promise.defer();
  Services.obs.addObserver(resolve, "performance-actors-connection-opened", false);
  return promise.then(() =>
    Services.obs.removeObserver(resolve, "performance-actors-connection-opened"));
}

function* teardown(panel) {
  info("Destroying the performance tool.");

  let tab = panel.target.tab;
  yield panel._toolbox.destroy();
  yield removeTab(tab);
}

function idleWait(time) {
  return DevToolsUtils.waitForTime(time);
}

function busyWait(time) {
  let start = Date.now();
  let stack;
  while (Date.now() - start < time) { stack = Components.stack; }
}

function consoleMethod (...args) {
  if (!mm) {
    throw new Error("`loadFrameScripts()` must be called before using frame scripts.");
  }
  
  
  
  
  
  if (args[1] == null) {
    args[1] = "";
  }
  mm.sendAsyncMessage("devtools:test:console", args);
}

function* consoleProfile(win, label) {
  let profileStart = once(win.PerformanceController, win.EVENTS.RECORDING_STARTED);
  consoleMethod("profile", label);
  yield profileStart;
}

function* consoleProfileEnd(win, label) {
  let ended = once(win.PerformanceController, win.EVENTS.RECORDING_STOPPED);
  consoleMethod("profileEnd", label);
  yield ended;
}

function command (button) {
  let ev = button.ownerDocument.createEvent("XULCommandEvent");
  ev.initCommandEvent("command", true, true, button.ownerDocument.defaultView, 0, false, false, false, false, null);
  button.dispatchEvent(ev);
}

function click (win, button) {
  EventUtils.sendMouseEvent({ type: "click" }, button, win);
}

function mousedown (win, button) {
  EventUtils.sendMouseEvent({ type: "mousedown" }, button, win);
}

function* startRecording(panel, options = {
  waitForOverview: true,
  waitForStateChanged: true
}) {
  let win = panel.panelWin;
  let clicked = panel.panelWin.PerformanceView.once(win.EVENTS.UI_START_RECORDING);
  let willStart = panel.panelWin.PerformanceController.once(win.EVENTS.RECORDING_WILL_START);
  let hasStarted = panel.panelWin.PerformanceController.once(win.EVENTS.RECORDING_STARTED);
  let button = win.$("#main-record-button");

  ok(!button.hasAttribute("checked"),
    "The record button should not be checked yet.");
  ok(!button.hasAttribute("locked"),
    "The record button should not be locked yet.");

  click(win, button);
  yield clicked;

  ok(button.hasAttribute("checked"),
    "The record button should now be checked.");
  ok(button.hasAttribute("locked"),
    "The record button should be locked.");

  yield willStart;
  let stateChanged = options.waitForStateChanged
    ? once(win.PerformanceView, win.EVENTS.UI_STATE_CHANGED)
    : Promise.resolve();

  yield hasStarted;

  let overviewRendered = options.waitForOverview
    ? once(win.OverviewView, win.EVENTS.OVERVIEW_RENDERED)
    : Promise.resolve();

  yield stateChanged;
  yield overviewRendered;

  is(win.PerformanceView.getState(), "recording",
    "The current state is 'recording'.");

  ok(button.hasAttribute("checked"),
    "The record button should still be checked.");
  ok(!button.hasAttribute("locked"),
    "The record button should not be locked.");
}

function* stopRecording(panel, options = {
  waitForOverview: true,
  waitForStateChanged: true
}) {
  let win = panel.panelWin;
  let clicked = panel.panelWin.PerformanceView.once(win.EVENTS.UI_STOP_RECORDING);
  let willStop = panel.panelWin.PerformanceController.once(win.EVENTS.RECORDING_WILL_STOP);
  let hasStopped = panel.panelWin.PerformanceController.once(win.EVENTS.RECORDING_STOPPED);
  let button = win.$("#main-record-button");
  let overviewRendered = null;

  ok(button.hasAttribute("checked"),
    "The record button should already be checked.");
  ok(!button.hasAttribute("locked"),
    "The record button should not be locked yet.");

  click(win, button);
  yield clicked;

  ok(!button.hasAttribute("checked"),
    "The record button should not be checked.");
  ok(button.hasAttribute("locked"),
    "The record button should be locked.");

  yield willStop;
  let stateChanged = options.waitForStateChanged
    ? once(win.PerformanceView, win.EVENTS.UI_STATE_CHANGED)
    : Promise.resolve();

  yield hasStopped;

  
  
  while (!overviewRendered && options.waitForOverview) {
    let [_, res] = yield onceSpread(win.OverviewView, win.EVENTS.OVERVIEW_RENDERED);
    if (res === win.FRAMERATE_GRAPH_HIGH_RES_INTERVAL) {
      overviewRendered = true;
    }
  }

  yield stateChanged;

  is(win.PerformanceView.getState(), "recorded",
    "The current state is 'recorded'.");

  ok(!button.hasAttribute("checked"),
    "The record button should not be checked.");
  ok(!button.hasAttribute("locked"),
    "The record button should not be locked.");
}

function waitForWidgetsRendered(panel) {
  let {
    EVENTS,
    OverviewView,
    WaterfallView,
    JsCallTreeView,
    JsFlameGraphView,
    MemoryCallTreeView,
    MemoryFlameGraphView,
  } = panel.panelWin;

  return Promise.all([
    once(OverviewView, EVENTS.MARKERS_GRAPH_RENDERED),
    once(OverviewView, EVENTS.MEMORY_GRAPH_RENDERED),
    once(OverviewView, EVENTS.FRAMERATE_GRAPH_RENDERED),
    once(OverviewView, EVENTS.OVERVIEW_RENDERED),
    once(WaterfallView, EVENTS.WATERFALL_RENDERED),
    once(JsCallTreeView, EVENTS.JS_CALL_TREE_RENDERED),
    once(JsFlameGraphView, EVENTS.JS_FLAMEGRAPH_RENDERED),
    once(MemoryCallTreeView, EVENTS.MEMORY_CALL_TREE_RENDERED),
    once(MemoryFlameGraphView, EVENTS.MEMORY_FLAMEGRAPH_RENDERED),
  ]);
}









function waitUntil(predicate, interval = 10) {
  if (predicate()) {
    return Promise.resolve(true);
  }
  let deferred = Promise.defer();
  setTimeout(function() {
    waitUntil(predicate).then(() => deferred.resolve(true));
  }, interval);
  return deferred.promise;
}



function dragStart(graph, x, y = 1) {
  x /= window.devicePixelRatio;
  y /= window.devicePixelRatio;
  graph._onMouseMove({ testX: x, testY: y });
  graph._onMouseDown({ testX: x, testY: y });
}

function dragStop(graph, x, y = 1) {
  x /= window.devicePixelRatio;
  y /= window.devicePixelRatio;
  graph._onMouseMove({ testX: x, testY: y });
  graph._onMouseUp({ testX: x, testY: y });
}

function dropSelection(graph) {
  graph.dropSelection();
  graph.emit("selecting");
}




function fireKey (e) {
  EventUtils.synthesizeKey(e, {});
}

function reload (aTarget, aEvent = "navigate") {
  aTarget.activeTab.reload();
  return once(aTarget, aEvent);
}




function forceCC () {
  info("Triggering GC/CC...");
  SpecialPowers.DOMWindowUtils.cycleCollect();
  SpecialPowers.DOMWindowUtils.garbageCollect();
  SpecialPowers.DOMWindowUtils.garbageCollect();
}
