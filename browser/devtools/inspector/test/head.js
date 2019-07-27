



"use strict";

const Cu = Components.utils;
const Ci = Components.interfaces;
const Cc = Components.classes;









const TEST_URL_ROOT = "http://example.com/browser/browser/devtools/inspector/test/";
const ROOT_TEST_DIR = getRootDirectory(gTestPath);
const FRAME_SCRIPT_URL = ROOT_TEST_DIR + "doc_frame_script.js";
const { Promise: promise } = Cu.import("resource://gre/modules/Promise.jsm", {});


waitForExplicitFinish();

let {TargetFactory, require} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools;
let {console} = Cu.import("resource://gre/modules/devtools/Console.jsm", {});


let testDir = gTestPath.substr(0, gTestPath.lastIndexOf("/"));
Services.scriptloader.loadSubScript(testDir + "../../../commandline/test/helpers.js", this);

gDevTools.testing = true;
registerCleanupFunction(() => {
  gDevTools.testing = false;
});

registerCleanupFunction(() => {
  Services.prefs.clearUserPref("devtools.dump.emit");
  Services.prefs.clearUserPref("devtools.inspector.activeSidebar");
});

registerCleanupFunction(function*() {
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  yield gDevTools.closeToolbox(target);

  
  
  
  
  EventUtils.synthesizeMouseAtPoint(1, 1, {type: "mousemove"}, window);

  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
});






let addTab = Task.async(function* (url) {
  info("Adding a new tab with URL: '" + url + "'");

  window.focus();

  let tab = gBrowser.selectedTab = gBrowser.addTab(url);
  let browser = tab.linkedBrowser;

  info("Loading the helper frame script " + FRAME_SCRIPT_URL);
  browser.messageManager.loadFrameScript(FRAME_SCRIPT_URL, false);

  yield once(browser, "load", true);
  info("URL '" + url + "' loading complete");

  return tab;
});
















function getNode(nodeOrSelector, options = {}) {
  let document = options.document || content.document;
  let noMatches = !!options.expectNoMatch;

  if (typeof nodeOrSelector === "string") {
    info("Looking for a node that matches selector " + nodeOrSelector);
    let node = document.querySelector(nodeOrSelector);
    if (noMatches) {
      ok(!node, "Selector " + nodeOrSelector + " didn't match any nodes.");
    }
    else {
      ok(node, "Selector " + nodeOrSelector + " matched a node.");
    }

    return node;
  }

  info("Looking for a node but selector was not a string.");
  return nodeOrSelector;
}










function selectAndHighlightNode(selector, inspector) {
  info("Highlighting and selecting the node " + selector);
  return selectNode(selector, inspector, "test-highlight");
}











let selectNode = Task.async(function*(selector, inspector, reason="test") {
  info("Selecting the node for '" + selector + "'");
  let nodeFront = yield getNodeFront(selector, inspector);
  let updated = inspector.once("inspector-updated");
  inspector.selection.setNodeFront(nodeFront, reason);
  yield updated;
});








let openInspectorForURL = Task.async(function*(url, hostType) {
  let tab = yield addTab(url);
  let { inspector, toolbox } = yield openInspector(null, hostType);
  return { tab, inspector, toolbox };
});








let openInspector = Task.async(function*(cb, hostType) {
  info("Opening the inspector");
  let target = TargetFactory.forTab(gBrowser.selectedTab);

  let inspector, toolbox;

  
  
  
  toolbox = gDevTools.getToolbox(target);
  if (toolbox) {
    inspector = toolbox.getPanel("inspector");
    if (inspector) {
      info("Toolbox and inspector already open");
      if (cb) {
        return cb(inspector, toolbox);
      } else {
        return {
          toolbox: toolbox,
          inspector: inspector
        };
      }
    }
  }

  info("Opening the toolbox");
  toolbox = yield gDevTools.showToolbox(target, "inspector", hostType);
  yield waitForToolboxFrameFocus(toolbox);
  inspector = toolbox.getPanel("inspector");

  info("Waiting for the inspector to update");
  yield inspector.once("inspector-updated");

  if (cb) {
    return cb(inspector, toolbox);
  } else {
    return {
      toolbox: toolbox,
      inspector: inspector
    };
  }
});






function waitForToolboxFrameFocus(toolbox) {
  info("Making sure that the toolbox's frame is focused");
  let def = promise.defer();
  let win = toolbox.frame.contentWindow;
  waitForFocus(def.resolve, win);
  return def.promise;
}

function getActiveInspector() {
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  return gDevTools.getToolbox(target).getPanel("inspector");
}









function getNodeFront(selector, {walker}) {
  if (selector._form) {
    return selector;
  }
  return walker.querySelector(walker.rootNode, selector);
}













let getNodeFrontInFrame = Task.async(function*(selector, frameSelector,
                                               inspector, reason="test") {
  let iframe = yield getNodeFront(frameSelector, inspector);
  let {nodes} = yield inspector.walker.children(iframe);
  return inspector.walker.querySelector(nodes[0], selector);
});




let getSimpleBorderRect = Task.async(function*(toolbox) {
  let {border} = yield getBoxModelStatus(toolbox);
  let {p1, p2, p3, p4} = border.points;

  return {
    top: p1.y,
    left: p1.x,
    width: p2.x - p1.x,
    height: p4.y - p1.y
  };
});

function getHighlighterActorID(highlighter) {
  let actorID = highlighter.actorID;
  let connPrefix = actorID.substring(0, actorID.indexOf(highlighter.typeName));

  return {actorID, connPrefix};
}





let getBoxModelStatus = Task.async(function*(toolbox) {
  let isVisible = yield isHighlighting(toolbox);

  let ret = {
    visible: isVisible
  };

  for (let region of ["margin", "border", "padding", "content"]) {
    let points = yield getPointsForRegion(region, toolbox);
    let visible = yield isRegionHidden(region, toolbox);
    ret[region] = {points, visible};
  }

  ret.guides = {};
  for (let guide of ["top", "right", "bottom", "left"]) {
    ret.guides[guide] = yield getGuideStatus(guide, toolbox);
  }

  return ret;
});









let getGuideStatus = Task.async(function*(location, {highlighter}) {
  let id = "box-model-guide-" + location;

  let hidden = yield getHighlighterNodeAttribute(highlighter, id, "hidden");
  let x1 = yield getHighlighterNodeAttribute(highlighter, id, "x1");
  let y1 = yield getHighlighterNodeAttribute(highlighter, id, "y1");
  let x2 = yield getHighlighterNodeAttribute(highlighter, id, "x2");
  let y2 = yield getHighlighterNodeAttribute(highlighter, id, "y2");

  return {
    visible: !hidden,
    x1: x1,
    y1: y1,
    x2: x2,
    y2: y2
  };
});








let getGuidesRectangle = Task.async(function*(toolbox) {
  let tGuide = yield getGuideStatus("top", toolbox);
  let rGuide = yield getGuideStatus("right", toolbox);
  let bGuide = yield getGuideStatus("bottom", toolbox);
  let lGuide = yield getGuideStatus("left", toolbox);

  if (!tGuide.visible || !rGuide.visible || !bGuide.visible || !lGuide.visible) {
    return null;
  }

  return {
    p1: {x: lGuide.x1, y: tGuide.y1},
    p2: {x: rGuide.x1, y: tGuide. y1},
    p3: {x: rGuide.x1, y: bGuide.y1},
    p4: {x: lGuide.x1, y: bGuide.y1}
  };
});





let getPointsForRegion = Task.async(function*(region, toolbox) {
  let d = yield getHighlighterNodeAttribute(toolbox.highlighter,
                                            "box-model-" + region, "d");
  let polygons = d.match(/M[^M]+/g);
  if (!polygons) {
    return null;
  }

  let points = polygons[0].trim().split(" ").map(i => {
    return i.replace(/M|L/, "").split(",")
  });

  return {
    p1: {
      x: parseFloat(points[0][0]),
      y: parseFloat(points[0][1])
    },
    p2: {
      x: parseFloat(points[1][0]),
      y: parseFloat(points[1][1])
    },
    p3: {
      x: parseFloat(points[2][0]),
      y: parseFloat(points[2][1])
    },
    p4: {
      x: parseFloat(points[3][0]),
      y: parseFloat(points[3][1])
    }
  };
});





let isRegionHidden = Task.async(function*(region, toolbox) {
  let value = yield getHighlighterNodeAttribute(toolbox.highlighter,
                                                "box-model-" + region, "hidden");
  return value !== null;
});




let isHighlighting = Task.async(function*(toolbox) {
  let value = yield getHighlighterNodeAttribute(toolbox.highlighter,
                                                "box-model-elements", "hidden");
  return value === null;
});

let getHighlitNode = Task.async(function*(toolbox) {
  let {visible, content} = yield getBoxModelStatus(toolbox);
  let points = content.points;
  if (visible) {
    let x = (points.p1.x + points.p2.x + points.p3.x + points.p4.x) / 4;
    let y = (points.p1.y + points.p2.y + points.p3.y + points.p4.y) / 4;

    let {objects} = yield executeInContent("Test:ElementFromPoint", {x, y});
    return objects.element;
  }
});








let isNodeCorrectlyHighlighted = Task.async(function*(selector, toolbox, prefix="") {
  let boxModel = yield getBoxModelStatus(toolbox);
  let {data: regions} = yield executeInContent("Test:GetAllAdjustedQuads",
                                               {selector});

  for (let boxType of ["content", "padding", "border", "margin"]) {
    let [quad] = regions[boxType];
    for (let point in boxModel[boxType].points) {
      is(boxModel[boxType].points[point].x, quad[point].x,
        "Node " + selector + " " + boxType + " point " + point +
        " x coordinate is correct");
      is(boxModel[boxType].points[point].y, quad[point].y,
        "Node " + selector + " " + boxType + " point " + point +
        " y coordinate is correct");
    }
  }
});

function synthesizeKeyFromKeyTag(aKeyId, aDocument = null) {
  let document = aDocument || document;
  let key = document.getElementById(aKeyId);
  isnot(key, null, "Successfully retrieved the <key> node");

  let modifiersAttr = key.getAttribute("modifiers");

  let name = null;

  if (key.getAttribute("keycode"))
    name = key.getAttribute("keycode");
  else if (key.getAttribute("key"))
    name = key.getAttribute("key");

  isnot(name, null, "Successfully retrieved keycode/key");

  let modifiers = {
    shiftKey: modifiersAttr.match("shift"),
    ctrlKey: modifiersAttr.match("ctrl"),
    altKey: modifiersAttr.match("alt"),
    metaKey: modifiersAttr.match("meta"),
    accelKey: modifiersAttr.match("accel")
  }

  EventUtils.synthesizeKey(name, modifiers);
}

let focusSearchBoxUsingShortcut = Task.async(function* (panelWin, callback) {
  info("Focusing search box");
  let searchBox = panelWin.document.getElementById("inspector-searchbox");
  let focused = once(searchBox, "focus");

  panelWin.focus();
  synthesizeKeyFromKeyTag("nodeSearchKey", panelWin.document);

  yield focused;

  if (callback) {
    callback();
  }
});









function getContainerForNodeFront(nodeFront, {markup}) {
  return markup.getContainer(nodeFront);
}









let getContainerForSelector = Task.async(function*(selector, inspector) {
  info("Getting the markup-container for node " + selector);
  let nodeFront = yield getNodeFront(selector, inspector);
  let container = getContainerForNodeFront(nodeFront, inspector);
  info("Found markup-container " + container);
  return container;
});










let hoverContainer = Task.async(function*(selector, inspector) {
  info("Hovering over the markup-container for node " + selector);

  let nodeFront = yield getNodeFront(selector, inspector);
  let container = getContainerForNodeFront(nodeFront, inspector);

  let highlit = inspector.toolbox.once("node-highlight");
  EventUtils.synthesizeMouseAtCenter(container.tagLine, {type: "mousemove"},
    inspector.markup.doc.defaultView);
  return highlit;
});









let clickContainer = Task.async(function*(selector, inspector) {
  info("Clicking on the markup-container for node " + selector);

  let nodeFront = yield getNodeFront(selector, inspector);
  let container = getContainerForNodeFront(nodeFront, inspector);

  let updated = inspector.once("inspector-updated");
  EventUtils.synthesizeMouseAtCenter(container.tagLine, {type: "mousedown"},
    inspector.markup.doc.defaultView);
  EventUtils.synthesizeMouseAtCenter(container.tagLine, {type: "mouseup"},
    inspector.markup.doc.defaultView);
  return updated;
});









let zoomPageTo = Task.async(function*(level, actorID, connPrefix) {
  yield executeInContent("Test:ChangeZoomLevel",
                         {level, actorID, connPrefix});
});








let getHighlighterNodeAttribute = Task.async(function*(highlighter, nodeID, name) {
  let {actorID, connPrefix} = getHighlighterActorID(highlighter);
  let {data: value} = yield executeInContent("Test:GetHighlighterAttribute",
                                             {nodeID, name, actorID, connPrefix});
  return value;
});











let getHighlighterRegionPath = Task.async(function*(region, highlighter) {
  let d = yield getHighlighterNodeAttribute(highlighter, "box-model-" + region, "d");
  if (!d) {
    return {d: null};
  }

  let polygons = d.match(/M[^M]+/g);
  if (!polygons) {
    return {d};
  }

  let points = [];
  for (let polygon of polygons) {
    points.push(polygon.trim().split(" ").map(i => {
      return i.replace(/M|L/, "").split(",")
    }));
  }

  return {d, points};
});







let getHighlighterNodeTextContent = Task.async(function*(highlighter, nodeID) {
  let {actorID, connPrefix} = getHighlighterActorID(highlighter);
  let {data: value} = yield executeInContent("Test:GetHighlighterTextContent",
                                             {nodeID, actorID, connPrefix});
  return value;
});








function waitForHighlighterEvent(event, highlighter) {
  let {actorID, connPrefix} = getHighlighterActorID(highlighter);
  return executeInContent("Test:WaitForHighlighterEvent",
                          {event, actorID, connPrefix});
}






function mouseLeaveMarkupView(inspector) {
  info("Leaving the markup-view area");
  let def = promise.defer();

  
  let btn = inspector.toolbox.doc.querySelector(".toolbox-dock-button");

  EventUtils.synthesizeMouseAtCenter(btn, {type: "mousemove"},
    inspector.toolbox.doc.defaultView);
  executeSoon(def.resolve);

  return def.promise;
}









function once(target, eventName, useCapture=false) {
  info("Waiting for event: '" + eventName + "' on " + target + ".");

  let deferred = promise.defer();

  for (let [add, remove] of [
    ["addEventListener", "removeEventListener"],
    ["addListener", "removeListener"],
    ["on", "off"]
  ]) {
    if ((add in target) && (remove in target)) {
      target[add](eventName, function onEvent(...aArgs) {
        info("Got event: '" + eventName + "' on " + target + ".");
        target[remove](eventName, onEvent, useCapture);
        deferred.resolve.apply(deferred, aArgs);
      }, useCapture);
      break;
    }
  }

  return deferred.promise;
}








function waitForContentMessage(name) {
  let mm = gBrowser.selectedBrowser.messageManager;

  let def = promise.defer();
  mm.addMessageListener(name, function onMessage(msg) {
    mm.removeMessageListener(name, onMessage);
    def.resolve(msg);
  });
  return def.promise;
}

function wait(ms) {
  let def = promise.defer();
  setTimeout(def.resolve, ms);
  return def.promise;
}













function executeInContent(name, data={}, objects={}, expectResponse=true) {
  let mm = gBrowser.selectedBrowser.messageManager;

  mm.sendAsyncMessage(name, data, objects);
  if (expectResponse) {
    return waitForContentMessage(name);
  } else {
    return promise.resolve();
  }
}









function undoChange(inspector) {
  let canUndo = inspector.markup.undo.canUndo();
  ok(canUndo, "The last change in the markup-view can be undone");
  if (!canUndo) {
    return promise.reject();
  }

  let mutated = inspector.once("markupmutation");
  inspector.markup.undo.undo();
  return mutated;
}









function redoChange(inspector) {
  let canRedo = inspector.markup.undo.canRedo();
  ok(canRedo, "The last change in the markup-view can be redone");
  if (!canRedo) {
    return promise.reject();
  }

  let mutated = inspector.once("markupmutation");
  inspector.markup.undo.redo();
  return mutated;
}
