



const Cu = Components.utils;
let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
let TargetFactory = devtools.TargetFactory;
let {console} = Cu.import("resource://gre/modules/devtools/Console.jsm", {});
let promise = devtools.require("resource://gre/modules/Promise.jsm").Promise;
let {getInplaceEditorForSpan: inplaceEditor} = devtools.require("devtools/shared/inplace-editor");
let clipboard = devtools.require("sdk/clipboard");
let {setTimeout, clearTimeout} = devtools.require("sdk/timers");


waitForExplicitFinish();



SimpleTest.requestCompleteLog();





gDevTools.testing = true;
registerCleanupFunction(() => gDevTools.testing = false);


registerCleanupFunction(() => {
  Services.prefs.clearUserPref("devtools.inspector.htmlPanelOpen");
  Services.prefs.clearUserPref("devtools.inspector.sidebarOpen");
  Services.prefs.clearUserPref("devtools.inspector.activeSidebar");
  Services.prefs.clearUserPref("devtools.dump.emit");
  Services.prefs.clearUserPref("devtools.markup.pagesize");
  Services.prefs.clearUserPref("dom.webcomponents.enabled");
  Services.prefs.clearUserPref("devtools.inspector.showAllAnonymousContent");
});


registerCleanupFunction(function*() {
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  yield gDevTools.closeToolbox(target);

  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
});

const TEST_URL_ROOT = "http://mochi.test:8888/browser/browser/devtools/markupview/test/";
const CHROME_BASE = "chrome://mochitests/content/browser/browser/devtools/markupview/test/";
const COMMON_FRAME_SCRIPT_URL = "chrome://browser/content/devtools/frame-script-utils.js";






function addTab(url) {
  info("Adding a new tab with URL: '" + url + "'");
  let def = promise.defer();

  
  
  
  window.focus();

  let tab = window.gBrowser.selectedTab = window.gBrowser.addTab(url);
  let linkedBrowser = tab.linkedBrowser;

  info("Loading the helper frame script " + COMMON_FRAME_SCRIPT_URL);
  linkedBrowser.messageManager.loadFrameScript(COMMON_FRAME_SCRIPT_URL, false);

  linkedBrowser.addEventListener("load", function onload() {
    linkedBrowser.removeEventListener("load", onload, true);
    info("URL '" + url + "' loading complete");
    def.resolve(tab);
  }, true);

  return def.promise;
}












function loadHelperScript(filePath) {
  let testDir = gTestPath.substr(0, gTestPath.lastIndexOf("/"));
  Services.scriptloader.loadSubScript(testDir + "/" + filePath, this);
}






function reloadPage(inspector) {
  info("Reloading the page");
  let newRoot = inspector.once("new-root");
  content.location.reload();
  return newRoot;
}





function openInspector() {
  info("Opening the inspector panel");
  let def = promise.defer();

  let target = TargetFactory.forTab(gBrowser.selectedTab);
  gDevTools.showToolbox(target, "inspector").then(function(toolbox) {
    info("The toolbox is open");
    let inspector = toolbox.getCurrentPanel();
    inspector.once("inspector-updated", () => {
      info("The inspector panel is active and ready");
      def.resolve({toolbox: toolbox, inspector: inspector});
    });
  }).then(null, console.error);

  return def.promise;
}








function waitForContentMessage(name) {
  info("Expecting message " + name + " from content");

  let mm = gBrowser.selectedBrowser.messageManager;

  let def = promise.defer();
  mm.addMessageListener(name, function onMessage(msg) {
    mm.removeMessageListener(name, onMessage);
    def.resolve(msg.data);
  });
  return def.promise;
}













function executeInContent(name, data={}, objects={}, expectResponse=true) {
  info("Sending message " + name + " to content");
  let mm = gBrowser.selectedBrowser.messageManager;

  mm.sendAsyncMessage(name, data, objects);
  if (expectResponse) {
    return waitForContentMessage(name);
  } else {
    return promise.resolve();
  }
}




function reloadTab() {
  return executeInContent("devtools:test:reload", {}, {}, false);
}








function getNode(nodeOrSelector) {
  info("Getting the node for '" + nodeOrSelector + "'");
  return typeof nodeOrSelector === "string" ?
    content.document.querySelector(nodeOrSelector) :
    nodeOrSelector;
}








function getNodeFront(selector, {walker}) {
  if (selector._form) {
    return selector;
  }
  return walker.querySelector(walker.rootNode, selector);
}






function getNodeInfo(selector) {
  return executeInContent("devtools:test:getDomElementInfo", {selector});
}








function setNodeAttribute(selector, attributeName, attributeValue) {
  return executeInContent("devtools:test:setAttribute",
                          {selector, attributeName, attributeValue});
}










function selectAndHighlightNode(nodeOrSelector, inspector) {
  info("Highlighting and selecting the node " + nodeOrSelector);

  let node = getNode(nodeOrSelector);
  let updated = inspector.toolbox.once("highlighter-ready");
  inspector.selection.setNode(node, "test-highlight");
  return updated;
}











let selectNode = Task.async(function*(selector, inspector, reason="test") {
  info("Selecting the node for '" + selector + "'");
  let nodeFront = yield getNodeFront(selector, inspector);
  let updated = inspector.once("inspector-updated");
  inspector.selection.setNodeFront(nodeFront, reason);
  yield updated;
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









function waitForChildrenUpdated({markup}) {
  info("Waiting for queued children updates to be handled");
  let def = promise.defer();
  markup._waitForChildren().then(() => {
    executeSoon(def.resolve);
  });
  return def.promise;
}









let clickContainer = Task.async(function*(selector, inspector) {
  info("Clicking on the markup-container for node " + selector);

  let nodeFront = yield getNodeFront(selector, inspector);
  let container = getContainerForNodeFront(nodeFront, inspector);

  let updated = container.selected ? promise.resolve() : inspector.once("inspector-updated");
  EventUtils.synthesizeMouseAtCenter(container.tagLine, {type: "mousedown"},
    inspector.markup.doc.defaultView);
  EventUtils.synthesizeMouseAtCenter(container.tagLine, {type: "mouseup"},
    inspector.markup.doc.defaultView);
  return updated;
});





function isHighlighterVisible() {
  let highlighter = gBrowser.selectedBrowser.parentNode
                            .querySelector(".highlighter-container .box-model-root");
  return highlighter && !highlighter.hasAttribute("hidden");
}









function setEditableFieldValue(field, value, inspector) {
  field.focus();
  EventUtils.sendKey("return", inspector.panelWin);
  let input = inplaceEditor(field).input;
  ok(input, "Found editable field for setting value: " + value);
  input.value = value;
  EventUtils.sendKey("return", inspector.panelWin);
}











let addNewAttributes = Task.async(function*(selector, text, inspector) {
  info("Entering text '" + text + "' in node '" + selector + "''s new attribute field");

  let container = yield getContainerForSelector(selector, inspector);
  ok(container, "The container for '" + selector + "' was found");

  info("Listening for the markupmutation event");
  let nodeMutated = inspector.once("markupmutation");
  setEditableFieldValue(container.editor.newAttr, text, inspector);
  yield nodeMutated;
});











let assertAttributes = Task.async(function*(selector, expected) {
  let {attributes: actual} = yield getNodeInfo(selector);

  is(actual.length, Object.keys(expected).length,
    "The node " + selector + " has the expected number of attributes.");
  for (let attr in expected) {
    let foundAttr = actual.find(({name, value}) => name === attr);
    let foundValue = foundAttr ? foundAttr.value : undefined;
    ok(foundAttr, "The node " + selector + " has the attribute " + attr);
    is(foundValue, expected[attr],
      "The node " + selector + " has the correct " + attr + " attribute value");
  }
});









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





function getSelectorSearchBox(inspector) {
  return inspector.panelWin.document.getElementById("inspector-searchbox");
}








function searchUsingSelectorSearch(selector, inspector) {
  info("Entering \"" + selector + "\" into the selector-search input field");
  let field = getSelectorSearchBox(inspector);
  field.focus();
  field.value = selector;
  EventUtils.sendKey("return", inspector.panelWin);
}







function wait(ms) {
  let def = promise.defer();
  content.setTimeout(def.resolve, ms);
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










let isEditingMenuDisabled = Task.async(function*(nodeFront, inspector, assert=true) {
  let deleteMenuItem = inspector.panelDoc.getElementById("node-menu-delete");
  let editHTMLMenuItem = inspector.panelDoc.getElementById("node-menu-edithtml");
  let pasteHTMLMenuItem = inspector.panelDoc.getElementById("node-menu-pasteouterhtml");

  
  clipboard.set("<p>test</p>", "html");

  let menu = inspector.nodemenu;
  yield selectNode(nodeFront, inspector);
  yield reopenMenu(menu);

  let isDeleteMenuDisabled = deleteMenuItem.hasAttribute("disabled");
  let isEditHTMLMenuDisabled = editHTMLMenuItem.hasAttribute("disabled");
  let isPasteHTMLMenuDisabled = pasteHTMLMenuItem.hasAttribute("disabled");

  if (assert) {
    ok(isDeleteMenuDisabled, "Delete menu item is disabled");
    ok(isEditHTMLMenuDisabled, "Edit HTML menu item is disabled");
    ok(isPasteHTMLMenuDisabled, "Paste HTML menu item is disabled");
  }

  return isDeleteMenuDisabled && isEditHTMLMenuDisabled && isPasteHTMLMenuDisabled;
});










let isEditingMenuEnabled = Task.async(function*(nodeFront, inspector, assert=true) {
  let deleteMenuItem = inspector.panelDoc.getElementById("node-menu-delete");
  let editHTMLMenuItem = inspector.panelDoc.getElementById("node-menu-edithtml");
  let pasteHTMLMenuItem = inspector.panelDoc.getElementById("node-menu-pasteouterhtml");

  
  clipboard.set("<p>test</p>", "html");

  let menu = inspector.nodemenu;
  yield selectNode(nodeFront, inspector);
  yield reopenMenu(menu);

  let isDeleteMenuDisabled = deleteMenuItem.hasAttribute("disabled");
  let isEditHTMLMenuDisabled = editHTMLMenuItem.hasAttribute("disabled");
  let isPasteHTMLMenuDisabled = pasteHTMLMenuItem.hasAttribute("disabled");

  if (assert) {
    ok(!isDeleteMenuDisabled, "Delete menu item is enabled");
    ok(!isEditHTMLMenuDisabled, "Edit HTML menu item is enabled");
    ok(!isPasteHTMLMenuDisabled, "Paste HTML menu item is enabled");
  }

  return !isDeleteMenuDisabled && !isEditHTMLMenuDisabled && !isPasteHTMLMenuDisabled;
});






let reopenMenu = Task.async(function*(menu) {
  
  if (menu.state == "closing" || menu.state == "open") {
    let popuphidden = once(menu, "popuphidden", true);
    menu.hidePopup();
    yield popuphidden;
  }

  
  let popupshown = once(menu, "popupshown", true);
  menu.openPopup();
  yield popupshown;
});





function promiseNextTick() {
  let deferred = promise.defer();
  executeSoon(deferred.resolve);
  return deferred.promise;
}





function collapseSelectionAndTab(inspector) {
  EventUtils.sendKey("tab", inspector.panelWin); 
  EventUtils.sendKey("tab", inspector.panelWin); 
}





function collapseSelectionAndShiftTab(inspector) {
  EventUtils.synthesizeKey("VK_TAB", { shiftKey: true },
    inspector.panelWin); 
  EventUtils.synthesizeKey("VK_TAB", { shiftKey: true },
    inspector.panelWin); 
}







function checkFocusedAttribute(attrName, editMode) {
  let focusedAttr = Services.focus.focusedElement;
  is(focusedAttr ? focusedAttr.parentNode.dataset.attr : undefined,
    attrName, attrName + " attribute editor is currently focused.");
  is(focusedAttr ? focusedAttr.tagName : undefined,
    editMode ? "input": "span",
    editMode ? attrName + " is in edit mode" : attrName + " is not in edit mode");
}




function* waitForMultipleChildrenUpdates(inspector) {
  
  
  if (inspector.markup._queuedChildUpdates &&
      inspector.markup._queuedChildUpdates.size) {
    yield waitForChildrenUpdated(inspector);
    return yield waitForMultipleChildrenUpdates(inspector);
  }
}
















function createTestHTTPServer() {
  const {HttpServer} = Cu.import("resource://testing-common/httpd.js", {});
  let server = new HttpServer();

  registerCleanupFunction(function* cleanup() {
    let destroyed = promise.defer();
    server.stop(() => {
      destroyed.resolve();
    });
    yield destroyed.promise;
  });

  server.start(-1);
  return server;
}
