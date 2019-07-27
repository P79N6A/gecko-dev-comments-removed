

"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

let { Services } = Cu.import("resource://gre/modules/Services.jsm", {});



let gEnableLogging = Services.prefs.getBoolPref("devtools.debugger.log");
Services.prefs.setBoolPref("devtools.debugger.log", true);

let { Task } = Cu.import("resource://gre/modules/Task.jsm", {});
let { Promise } = Cu.import("resource://gre/modules/Promise.jsm", {});
let { gDevTools } = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});
let { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
let { DebuggerServer } = Cu.import("resource://gre/modules/devtools/dbg-server.jsm", {});

let { WebAudioFront } = devtools.require("devtools/server/actors/webaudio");
let TargetFactory = devtools.TargetFactory;

const EXAMPLE_URL = "http://example.com/browser/browser/devtools/webaudioeditor/test/";
const SIMPLE_CONTEXT_URL = EXAMPLE_URL + "doc_simple-context.html";
const COMPLEX_CONTEXT_URL = EXAMPLE_URL + "doc_complex-context.html";
const SIMPLE_NODES_URL = EXAMPLE_URL + "doc_simple-node-creation.html";
const MEDIA_NODES_URL = EXAMPLE_URL + "doc_media-node-creation.html";
const BUFFER_AND_ARRAY_URL = EXAMPLE_URL + "doc_buffer-and-array.html";
const DESTROY_NODES_URL = EXAMPLE_URL + "doc_destroy-nodes.html";
const CONNECT_TOGGLE_URL = EXAMPLE_URL + "doc_connect-toggle.html";
const CONNECT_TOGGLE_PARAM_URL = EXAMPLE_URL + "doc_connect-toggle-param.html";
const CONNECT_PARAM_URL = EXAMPLE_URL + "doc_connect-param.html";
const CONNECT_MULTI_PARAM_URL = EXAMPLE_URL + "doc_connect-multi-param.html";
const IFRAME_CONTEXT_URL = EXAMPLE_URL + "doc_iframe-context.html";


waitForExplicitFinish();

let gToolEnabled = Services.prefs.getBoolPref("devtools.webaudioeditor.enabled");

gDevTools.testing = true;

registerCleanupFunction(() => {
  gDevTools.testing = false;
  info("finish() was called, cleaning up...");
  Services.prefs.setBoolPref("devtools.debugger.log", gEnableLogging);
  Services.prefs.setBoolPref("devtools.webaudioeditor.enabled", gToolEnabled);
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

function once(aTarget, aEventName, aUseCapture = false) {
  info("Waiting for event: '" + aEventName + "' on " + aTarget + ".");

  let deferred = Promise.defer();

  for (let [add, remove] of [
    ["on", "off"], 
    ["addEventListener", "removeEventListener"],
    ["addListener", "removeListener"]
  ]) {
    if ((add in aTarget) && (remove in aTarget)) {
      aTarget[add](aEventName, function onEvent(...aArgs) {
        aTarget[remove](aEventName, onEvent, aUseCapture);
        deferred.resolve(...aArgs);
      }, aUseCapture);
      break;
    }
  }

  return deferred.promise;
}

function reload(aTarget, aWaitForTargetEvent = "navigate") {
  aTarget.activeTab.reload();
  return once(aTarget, aWaitForTargetEvent);
}

function navigate(aTarget, aUrl, aWaitForTargetEvent = "navigate") {
  executeSoon(() => aTarget.activeTab.navigateTo(aUrl));
  return once(aTarget, aWaitForTargetEvent);
}

function test () {
  Task.spawn(spawnTest).then(finish, handleError);
}

function initBackend(aUrl) {
  info("Initializing a web audio editor front.");

  if (!DebuggerServer.initialized) {
    DebuggerServer.init(() => true);
    DebuggerServer.addBrowserActors();
  }

  return Task.spawn(function*() {
    let tab = yield addTab(aUrl);
    let target = TargetFactory.forTab(tab);

    yield target.makeRemote();

    let front = new WebAudioFront(target.client, target.form);
    return { target, front };
  });
}

function initWebAudioEditor(aUrl) {
  info("Initializing a web audio editor pane.");

  return Task.spawn(function*() {
    let tab = yield addTab(aUrl);
    let target = TargetFactory.forTab(tab);

    yield target.makeRemote();

    Services.prefs.setBoolPref("devtools.webaudioeditor.enabled", true);
    let toolbox = yield gDevTools.showToolbox(target, "webaudioeditor");
    let panel = toolbox.getCurrentPanel();
    return { target, panel, toolbox };
  });
}

function teardown(aPanel) {
  info("Destroying the web audio editor.");

  return Promise.all([
    once(aPanel, "destroyed"),
    removeTab(aPanel.target.tab)
  ]).then(() => {
    let gBrowser = window.gBrowser;
    while (gBrowser.tabs.length > 1) {
      gBrowser.removeCurrentTab();
    }
    gBrowser = null;
  });
}








function getN (front, eventName, count, spread) {
  let actors = [];
  let deferred = Promise.defer();
  front.on(eventName, function onEvent (...args) {
    let actor = args[0];
    if (actors.length !== count) {
      actors.push(spread ? args : actor);
    }
    if (actors.length === count) {
      front.off(eventName, onEvent);
      deferred.resolve(actors);
    }
  });
  return deferred.promise;
}

function get (front, eventName) { return getN(front, eventName, 1); }
function get2 (front, eventName) { return getN(front, eventName, 2); }
function get3 (front, eventName) { return getN(front, eventName, 3); }
function getSpread (front, eventName) { return getN(front, eventName, 1, true); }
function get2Spread (front, eventName) { return getN(front, eventName, 2, true); }
function get3Spread (front, eventName) { return getN(front, eventName, 3, true); }
function getNSpread (front, eventName, count) { return getN(front, eventName, count, true); }






function waitForGraphRendered (front, nodeCount, edgeCount, paramEdgeCount) {
  let deferred = Promise.defer();
  let eventName = front.EVENTS.UI_GRAPH_RENDERED;
  front.on(eventName, function onGraphRendered (_, nodes, edges, pEdges) {
    let paramEdgesDone = paramEdgeCount != null ? paramEdgeCount === pEdges : true;
    if (nodes === nodeCount && edges === edgeCount && paramEdgesDone) {
      front.off(eventName, onGraphRendered);
      deferred.resolve();
    }
  });
  return deferred.promise;
}

function checkVariableView (view, index, hash, description = "") {
  info("Checking Variable View");
  let scope = view.getScopeAtIndex(index);
  let variables = Object.keys(hash);

  
  
  if (!variables.length) {
    ok(isVisible(scope.window.$("#properties-tabpanel-content-empty")),
      description + " should show the empty properties tab.");
    return;
  }

  
  variables.forEach(variable => {
    let aVar = scope.get(variable);
    is(aVar.target.querySelector(".name").getAttribute("value"), variable,
      "Correct property name for " + variable);
    let value = aVar.target.querySelector(".value").getAttribute("value");

    
    
    
    try {
      value = JSON.parse(value);
    }
    catch (e) {}
    if (typeof hash[variable] === "function") {
      ok(hash[variable](value),
        "Passing property value of " + value + " for " + variable + " " + description);
    }
    else {
      ise(value, hash[variable],
        "Correct property value of " + hash[variable] + " for " + variable + " " + description);
    }
  });
}

function modifyVariableView (win, view, index, prop, value) {
  let deferred = Promise.defer();
  let scope = view.getScopeAtIndex(index);
  let aVar = scope.get(prop);
  scope.expand();

  win.on(win.EVENTS.UI_SET_PARAM, handleSetting);
  win.on(win.EVENTS.UI_SET_PARAM_ERROR, handleSetting);

  
  win.focus();
  aVar.focus();
  EventUtils.sendKey("RETURN", win);

  
  
  executeSoon(() => {
    info("Setting " + value + " for " + prop + "....");
    for (let c of (value + "")) {
      EventUtils.synthesizeKey(c, {}, win);
    }
    EventUtils.sendKey("RETURN", win);
  });

  function handleSetting (eventName) {
    win.off(win.EVENTS.UI_SET_PARAM, handleSetting);
    win.off(win.EVENTS.UI_SET_PARAM_ERROR, handleSetting);
    if (eventName === win.EVENTS.UI_SET_PARAM)
      deferred.resolve();
    if (eventName === win.EVENTS.UI_SET_PARAM_ERROR)
      deferred.reject();
  }

  return deferred.promise;
}

function findGraphEdge (win, source, target, param) {
  let selector = ".edgePaths .edgePath[data-source='" + source + "'][data-target='" + target + "']";
  if (param) {
    selector += "[data-param='" + param + "']";
  }
  return win.document.querySelector(selector);
}

function findGraphNode (win, node) {
  let selector = ".nodes > g[data-id='" + node + "']";
  return win.document.querySelector(selector);
}

function click (win, element) {
  EventUtils.sendMouseEvent({ type: "click" }, element, win);
}

function mouseOver (win, element) {
  EventUtils.sendMouseEvent({ type: "mouseover" }, element, win);
}

function isVisible (element) {
  return !element.getAttribute("hidden");
}




function wait (n) {
  let { promise, resolve } = Promise.defer();
  setTimeout(resolve, n);
  info("Waiting " + n/1000 + " seconds.");
  return promise;
}






function clickGraphNode (panelWin, el, waitForToggle = false) {
  let { promise, resolve } = Promise.defer();
  let promises = [
   once(panelWin, panelWin.EVENTS.UI_INSPECTOR_NODE_SET)
  ];

  if (waitForToggle) {
    promises.push(once(panelWin, panelWin.EVENTS.UI_INSPECTOR_TOGGLED));
  }

  
  
  let element = el.tagName ? el : findGraphNode(panelWin, el);
  click(panelWin, element);

  return Promise.all(promises);
}





function getGripValue (value) {
  if (~["boolean", "string", "number"].indexOf(typeof value)) {
    return value;
  }

  switch (value.type) {
    case "undefined": return undefined;
    case "Infinity": return Infinity;
    case "-Infinity": return -Infinity;
    case "NaN": return NaN;
    case "-0": return -0;
    case "null": return null;
    default: return value;
  }
}




function countGraphObjects (win) {
  return {
    nodes: win.document.querySelectorAll(".nodes > .audionode").length,
    edges: win.document.querySelectorAll(".edgePaths > .edgePath").length
  }
}




function forceCC () {
  SpecialPowers.DOMWindowUtils.cycleCollect();
  SpecialPowers.DOMWindowUtils.garbageCollect();
  SpecialPowers.DOMWindowUtils.garbageCollect();
}





const NODE_DEFAULT_VALUES = {
  "AudioDestinationNode": {},
  "MediaElementAudioSourceNode": {},
  "MediaStreamAudioSourceNode": {},
  "MediaStreamAudioDestinationNode": {
    "stream": "MediaStream"
  },
  "AudioBufferSourceNode": {
    "playbackRate": 1,
    "loop": false,
    "loopStart": 0,
    "loopEnd": 0,
    "buffer": null
  },
  "ScriptProcessorNode": {
    "bufferSize": 4096
  },
  "AnalyserNode": {
    "fftSize": 2048,
    "minDecibels": -100,
    "maxDecibels": -30,
    "smoothingTimeConstant": 0.8,
    "frequencyBinCount": 1024
  },
  "GainNode": {
    "gain": 1
  },
  "DelayNode": {
    "delayTime": 0
  },
  "BiquadFilterNode": {
    "type": "lowpass",
    "frequency": 350,
    "Q": 1,
    "detune": 0,
    "gain": 0
  },
  "WaveShaperNode": {
    "curve": null,
    "oversample": "none"
  },
  "PannerNode": {
    "panningModel": "HRTF",
    "distanceModel": "inverse",
    "refDistance": 1,
    "maxDistance": 10000,
    "rolloffFactor": 1,
    "coneInnerAngle": 360,
    "coneOuterAngle": 360,
    "coneOuterGain": 0
  },
  "ConvolverNode": {
    "buffer": null,
    "normalize": true
  },
  "ChannelSplitterNode": {},
  "ChannelMergerNode": {},
  "DynamicsCompressorNode": {
    "threshold": -24,
    "knee": 30,
    "ratio": 12,
    "reduction": 0,
    "attack": 0.003000000026077032,
    "release": 0.25
  },
  "OscillatorNode": {
    "type": "sine",
    "frequency": 440,
    "detune": 0
  }
};
