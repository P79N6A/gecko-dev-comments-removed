



const Cu = Components.utils;
const Ci = Components.interfaces;
const Cc = Components.classes;








let tempScope = {};
Cu.import("resource://gre/modules/devtools/LayoutHelpers.jsm", tempScope);
let LayoutHelpers = tempScope.LayoutHelpers;

let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", tempScope);
let TargetFactory = devtools.TargetFactory;

Components.utils.import("resource://gre/modules/devtools/Console.jsm", tempScope);
let console = tempScope.console;


let testDir = gTestPath.substr(0, gTestPath.lastIndexOf("/"));
Services.scriptloader.loadSubScript(testDir + "../../../commandline/test/helpers.js", this);

gDevTools.testing = true;
SimpleTest.registerCleanupFunction(() => {
  gDevTools.testing = false;
});

SimpleTest.registerCleanupFunction(() => {
  console.error("Here we are\n");
  let {DebuggerServer} = Cu.import("resource://gre/modules/devtools/dbg-server.jsm", {});
  console.error("DebuggerServer open connections: " + Object.getOwnPropertyNames(DebuggerServer._connections).length);

  Services.prefs.clearUserPref("devtools.dump.emit");
  Services.prefs.clearUserPref("devtools.inspector.activeSidebar");
});







function getNode(nodeOrSelector) {
  return typeof nodeOrSelector === "string" ?
    content.document.querySelector(nodeOrSelector) :
    nodeOrSelector;
}












function selectNode(nodeOrSelector, inspector, reason="test") {
  info("Selecting the node " + nodeOrSelector);
  let node = getNode(nodeOrSelector);
  let updated = inspector.once("inspector-updated");
  inspector.selection.setNode(node, reason);
  return updated;
}







let openInspector = Task.async(function*(cb) {
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
  toolbox = yield gDevTools.showToolbox(target, "inspector");
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







let openInspectorSideBar = Task.async(function*(id) {
  let {toolbox, inspector} = yield openInspector();

  if (!hasSideBarTab(inspector, id)) {
    info("Waiting for the " + id + " sidebar to be ready");
    yield inspector.sidebar.once(id + "-ready");
  }

  info("Selecting the " + id + " sidebar");
  inspector.sidebar.select(id);

  return {
    toolbox: toolbox,
    inspector: inspector,
    view: inspector.sidebar.getWindowForTab(id)[id].view
  };
});







function openComputedView() {
  return openInspectorSideBar("computedview");
}







function openRuleView() {
  return openInspectorSideBar("ruleview");
}








function hasSideBarTab(inspector, id) {
  return !!inspector.sidebar.getWindowForTab(id);
}

function getActiveInspector()
{
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  return gDevTools.getToolbox(target).getPanel("inspector");
}

function getNodeFront(node)
{
  let inspector = getActiveInspector();
  return inspector.walker.frontForRawNode(node);
}

function getHighlighter()
{
  return gBrowser.selectedBrowser.parentNode.querySelector(".highlighter-container");
}

function getSimpleBorderRect() {
  let {p1, p2, p3, p4} = getBoxModelStatus().border.points;

  return {
    top: p1.y,
    left: p1.x,
    width: p2.x - p1.x,
    height: p4.y - p1.y
  };
}

function getBoxModelRoot() {
  let highlighter = getHighlighter();
  return highlighter.querySelector(".box-model-root");
}

function getBoxModelStatus() {
  let root = getBoxModelRoot();
  let inspector = getActiveInspector();

  return {
    visible: !root.hasAttribute("hidden"),
    currentNode: inspector.walker.currentNode,
    margin: {
      points: getPointsForRegion("margin"),
      visible: isRegionHidden("margin")
    },
    border: {
      points: getPointsForRegion("border"),
      visible: isRegionHidden("border")
    },
    padding: {
      points: getPointsForRegion("padding"),
      visible: isRegionHidden("padding")
    },
    content: {
      points: getPointsForRegion("content"),
      visible: isRegionHidden("content")
    },
    guides: {
      top: getGuideStatus("top"),
      right: getGuideStatus("right"),
      bottom: getGuideStatus("bottom"),
      left: getGuideStatus("left")
    }
  };
}

function getGuideStatus(location) {
  let root = getBoxModelRoot();
  let guide = root.querySelector(".box-model-guide-" + location);

  return {
    visible: !guide.hasAttribute("hidden"),
    x1: guide.getAttribute("x1"),
    y1: guide.getAttribute("y1"),
    x2: guide.getAttribute("x2"),
    y2: guide.getAttribute("y2")
  };
}

function getPointsForRegion(region) {
  let root = getBoxModelRoot();
  let box = root.querySelector(".box-model-" + region);
  let points = box.getAttribute("points").split(/[, ]/);

  
  return {
    p1: {
      x: parseFloat(points[0]),
      y: parseFloat(points[1])
    },
    p2: {
      x: parseFloat(points[2]),
      y: parseFloat(points[3])
    },
    p3: {
      x: parseFloat(points[4]),
      y: parseFloat(points[5])
    },
    p4: {
      x: parseFloat(points[6]),
      y: parseFloat(points[7])
    }
  };
}

function isRegionHidden(region) {
  let root = getBoxModelRoot();
  let box = root.querySelector(".box-model-" + region);

  return !box.hasAttribute("hidden");
}

function isHighlighting()
{
  let root = getBoxModelRoot();
  return !root.hasAttribute("hidden");
}

function getHighlitNode()
{
  if (isHighlighting()) {
    let helper = new LayoutHelpers(window.content);
    let points = getBoxModelStatus().content.points;
    let x = (points.p1.x + points.p2.x + points.p3.x + points.p4.x) / 4;
    let y = (points.p1.y + points.p2.y + points.p3.y + points.p4.y) / 4;

    return helper.getElementFromPoint(window.content.document, x, y);
  }
}

function computedView()
{
  let sidebar = getActiveInspector().sidebar;
  let iframe = sidebar.tabbox.querySelector(".iframe-computedview");
  return iframe.contentWindow.computedView;
}

function computedViewTree()
{
  return computedView().view;
}

function ruleView()
{
  let sidebar = getActiveInspector().sidebar;
  let iframe = sidebar.tabbox.querySelector(".iframe-ruleview");
  return iframe.contentWindow.ruleView;
}

function getComputedView() {
  let inspector = getActiveInspector();
  return inspector.sidebar.getWindowForTab("computedview").computedview.view;
}

function waitForView(aName, aCallback) {
  let inspector = getActiveInspector();
  if (inspector.sidebar.getTab(aName)) {
    aCallback();
  } else {
    inspector.sidebar.once(aName + "-ready", aCallback);
  }
}

function synthesizeKeyFromKeyTag(aKeyId) {
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

function focusSearchBoxUsingShortcut(panelWin, callback) {
  panelWin.focus();
  let key = panelWin.document.getElementById("nodeSearchKey");
  isnot(key, null, "Successfully retrieved the <key> node");

  let modifiersAttr = key.getAttribute("modifiers");

  let name = null;

  if (key.getAttribute("keycode")) {
    name = key.getAttribute("keycode");
  } else if (key.getAttribute("key")) {
    name = key.getAttribute("key");
  }

  isnot(name, null, "Successfully retrieved keycode/key");

  let modifiers = {
    shiftKey: modifiersAttr.match("shift"),
    ctrlKey: modifiersAttr.match("ctrl"),
    altKey: modifiersAttr.match("alt"),
    metaKey: modifiersAttr.match("meta"),
    accelKey: modifiersAttr.match("accel")
  };

  let searchBox = panelWin.document.getElementById("inspector-searchbox");
  searchBox.addEventListener("focus", function onFocus() {
    searchBox.removeEventListener("focus", onFocus, false);
    callback && callback();
  }, false);
  EventUtils.synthesizeKey(name, modifiers);
}

function getComputedPropertyValue(aName)
{
  let computedview = getComputedView();
  let props = computedview.styleDocument.querySelectorAll(".property-view");

  for (let prop of props) {
    let name = prop.querySelector(".property-name");

    if (name.textContent === aName) {
      let value = prop.querySelector(".property-value");
      return value.textContent;
    }
  }
}

function isNodeCorrectlyHighlighted(node, prefix="") {
  let boxModel = getBoxModelStatus();
  let helper = new LayoutHelpers(window.content);

  prefix += (prefix ? " " : "") + node.nodeName;
  prefix += (node.id ? "#" + node.id : "");
  prefix += (node.classList.length ? "." + [...node.classList].join(".") : "");
  prefix += " ";

  let quads = helper.getAdjustedQuads(node, "content");
  let {p1:cp1, p2:cp2, p3:cp3, p4:cp4} = boxModel.content.points;
  is(cp1.x, quads.p1.x, prefix + "content point 1 x co-ordinate is correct");
  is(cp1.y, quads.p1.y, prefix + "content point 1 y co-ordinate is correct");
  is(cp2.x, quads.p2.x, prefix + "content point 2 x co-ordinate is correct");
  is(cp2.y, quads.p2.y, prefix + "content point 2 y co-ordinate is correct");
  is(cp3.x, quads.p3.x, prefix + "content point 3 x co-ordinate is correct");
  is(cp3.y, quads.p3.y, prefix + "content point 3 y co-ordinate is correct");
  is(cp4.x, quads.p4.x, prefix + "content point 4 x co-ordinate is correct");
  is(cp4.y, quads.p4.y, prefix + "content point 4 y co-ordinate is correct");

  quads = helper.getAdjustedQuads(node, "padding");
  let {p1:pp1, p2:pp2, p3:pp3, p4:pp4} = boxModel.padding.points;
  is(pp1.x, quads.p1.x, prefix + "padding point 1 x co-ordinate is correct");
  is(pp1.y, quads.p1.y, prefix + "padding point 1 y co-ordinate is correct");
  is(pp2.x, quads.p2.x, prefix + "padding point 2 x co-ordinate is correct");
  is(pp2.y, quads.p2.y, prefix + "padding point 2 y co-ordinate is correct");
  is(pp3.x, quads.p3.x, prefix + "padding point 3 x co-ordinate is correct");
  is(pp3.y, quads.p3.y, prefix + "padding point 3 y co-ordinate is correct");
  is(pp4.x, quads.p4.x, prefix + "padding point 4 x co-ordinate is correct");
  is(pp4.y, quads.p4.y, prefix + "padding point 4 y co-ordinate is correct");

  quads = helper.getAdjustedQuads(node, "border");
  let {p1:bp1, p2:bp2, p3:bp3, p4:bp4} = boxModel.border.points;
  is(bp1.x, quads.p1.x, prefix + "border point 1 x co-ordinate is correct");
  is(bp1.y, quads.p1.y, prefix + "border point 1 y co-ordinate is correct");
  is(bp2.x, quads.p2.x, prefix + "border point 2 x co-ordinate is correct");
  is(bp2.y, quads.p2.y, prefix + "border point 2 y co-ordinate is correct");
  is(bp3.x, quads.p3.x, prefix + "border point 3 x co-ordinate is correct");
  is(bp3.y, quads.p3.y, prefix + "border point 3 y co-ordinate is correct");
  is(bp4.x, quads.p4.x, prefix + "border point 4 x co-ordinate is correct");
  is(bp4.y, quads.p4.y, prefix + "border point 4 y co-ordinate is correct");

  quads = helper.getAdjustedQuads(node, "margin");
  let {p1:mp1, p2:mp2, p3:mp3, p4:mp4} = boxModel.margin.points;
  is(mp1.x, quads.p1.x, prefix + "margin point 1 x co-ordinate is correct");
  is(mp1.y, quads.p1.y, prefix + "margin point 1 y co-ordinate is correct");
  is(mp2.x, quads.p2.x, prefix + "margin point 2 x co-ordinate is correct");
  is(mp2.y, quads.p2.y, prefix + "margin point 2 y co-ordinate is correct");
  is(mp3.x, quads.p3.x, prefix + "margin point 3 x co-ordinate is correct");
  is(mp3.y, quads.p3.y, prefix + "margin point 3 y co-ordinate is correct");
  is(mp4.x, quads.p4.x, prefix + "margin point 4 x co-ordinate is correct");
  is(mp4.y, quads.p4.y, prefix + "margin point 4 y co-ordinate is correct");
}

function getContainerForRawNode(markupView, rawNode)
{
  let front = markupView.walker.frontForRawNode(rawNode);
  let container = markupView.getContainer(front);
  return container;
}

SimpleTest.registerCleanupFunction(function () {
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  gDevTools.closeToolbox(target);
});
