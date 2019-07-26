



const Cu = Components.utils;
let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
let TargetFactory = devtools.TargetFactory;
let {console} = Cu.import("resource://gre/modules/devtools/Console.jsm", {});
let promise = devtools.require("sdk/core/promise");
let {getInplaceEditorForSpan: inplaceEditor} = devtools.require("devtools/shared/inplace-editor");


waitForExplicitFinish();




gDevTools.testing = true;
registerCleanupFunction(() => gDevTools.testing = false);


registerCleanupFunction(() => {
  Services.prefs.clearUserPref("devtools.inspector.htmlPanelOpen");
  Services.prefs.clearUserPref("devtools.inspector.sidebarOpen");
  Services.prefs.clearUserPref("devtools.inspector.activeSidebar");
  Services.prefs.clearUserPref("devtools.dump.emit");
  Services.prefs.clearUserPref("devtools.markup.pagesize");
});


registerCleanupFunction(() => {
  try {
    let target = TargetFactory.forTab(gBrowser.selectedTab);
    gDevTools.closeToolbox(target);
  } catch (ex) {
    dump(ex);
  }
  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
});

const TEST_URL_ROOT = "http://mochi.test:8888/browser/browser/devtools/markupview/test/";




function asyncTest(generator) {
  return () => Task.spawn(generator).then(null, ok.bind(null, false)).then(finish);
}






function addTab(url) {
  info("Adding a new tab with URL: '" + url + "'");
  let def = promise.defer();

  let tab = gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    info("URL '" + url + "' loading complete");
    waitForFocus(() => {
      def.resolve(tab);
    }, content);
  }, true);
  content.location = url;

  return def.promise;
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







function getNode(nodeOrSelector) {
  info("Getting the node for '" + nodeOrSelector + "'");
  return typeof nodeOrSelector === "string" ?
    content.document.querySelector(nodeOrSelector) :
    nodeOrSelector;
}










function selectNode(nodeOrSelector, inspector, reason="test") {
  info("Selecting the node for '" + nodeOrSelector + "'");
  let node = getNode(nodeOrSelector);
  let updated = inspector.once("inspector-updated");
  inspector.selection.setNode(node, reason);
  return updated;
}










function getContainerForRawNode(nodeOrSelector, {markup}) {
  let front = markup.walker.frontForRawNode(getNode(nodeOrSelector));
  let container = markup.getContainer(front);
  info("Markup-container object for " + nodeOrSelector + " " + container);
  return container;
}









function hoverContainer(nodeOrSelector, inspector) {
  info("Hovering over the markup-container for node " + nodeOrSelector);
  let highlit = inspector.toolbox.once("node-highlight");
  let container = getContainerForRawNode(getNode(nodeOrSelector), inspector);
  EventUtils.synthesizeMouseAtCenter(container.tagLine, {type: "mousemove"},
    inspector.markup.doc.defaultView);
  return highlit;
}








function clickContainer(nodeOrSelector, inspector) {
  info("Clicking on the markup-container for node " + nodeOrSelector);
  let updated = inspector.once("inspector-updated");
  let container = getContainerForRawNode(getNode(nodeOrSelector), inspector);
  EventUtils.synthesizeMouseAtCenter(container.tagLine, {type: "mousedown"},
    inspector.markup.doc.defaultView);
  EventUtils.synthesizeMouseAtCenter(container.tagLine, {type: "mouseup"},
    inspector.markup.doc.defaultView);
  return updated;
}





function isHighlighterVisible() {
  let highlighter = gBrowser.selectedBrowser.parentNode
                            .querySelector(".highlighter-container .box-model-root");
  return highlighter && !highlighter.hasAttribute("hidden");
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









function setEditableFieldValue(field, value, inspector) {
  field.focus();
  EventUtils.sendKey("return", inspector.panelWin);
  let input = inplaceEditor(field).input;
  ok(input, "Found editable field for setting value: " + value);
  input.value = value;
  EventUtils.sendKey("return", inspector.panelWin);
}











function addNewAttributes(nodeOrSelector, text, inspector) {
  info("Entering text '" + text + "' in node '" + nodeOrSelector + "''s new attribute field");

  let container = getContainerForRawNode(nodeOrSelector, inspector);
  ok(container, "The container for '" + nodeOrSelector + "' was found");

  info("Listening for the markupmutation event");
  let nodeMutated = inspector.once("markupmutation");
  setEditableFieldValue(container.editor.newAttr, text, inspector);
  return nodeMutated;
}











function assertAttributes(nodeOrSelector, attrs) {
  let node = getNode(nodeOrSelector);

  is(node.attributes.length, Object.keys(attrs).length,
    "Node has the correct number of attributes.");
  for (let attr in attrs) {
    is(node.getAttribute(attr), attrs[attr],
      "Node has the correct " + attr + " attribute.");
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
















function runAddAttributesTests(tests, nodeOrSelector, inspector) {
  info("Running " + tests.length + " add-attributes tests");
  return Task.spawn(function*() {
    info("Selecting the test node");
    let div = getNode("div");
    yield selectNode(div, inspector);

    for (let test of tests) {
      yield runAddAttributesTest(test, div, inspector);
    }

    yield inspector.once("inspector-updated");
  });
}






















function* runAddAttributesTest(test, nodeOrSelector, inspector) {
  let element = getNode(nodeOrSelector);

  info("Starting add-attribute test: " + test.desc);
  yield addNewAttributes(element, test.text, inspector);

  info("Assert that the attribute(s) has/have been applied correctly");
  assertAttributes(element, test.expectedAttributes);

  if (test.validate) {
    test.validate(element, getContainerForRawNode(element, inspector), inspector);
  }

  info("Undo the change");
  yield undoChange(inspector);

  info("Assert that the attribute(s) has/have been removed correctly");
  assertAttributes(element, {});
}














function runEditAttributesTests(tests, inspector) {
  info("Running " + tests.length + " edit-attributes tests");
  return Task.spawn(function*() {
    info("Expanding all nodes in the markup-view");
    yield inspector.markup.expandAll();

    for (let test of tests) {
      yield runEditAttributesTest(test, inspector);
    }

    yield inspector.once("inspector-updated");
  });
}


















function* runEditAttributesTest(test, inspector) {
  info("Starting edit-attribute test: " + test.desc);

  info("Selecting the test node " + test.node);
  yield selectNode(test.node, inspector);

  info("Asserting that the node has the right attributes to start with");
  assertAttributes(test.node, test.originalAttributes);

  info("Editing attribute " + test.name + " with value " + test.value);

  let container = getContainerForRawNode(test.node, inspector);
  ok(container && container.editor, "The markup-container for " + test.node +
    " was found");

  info("Listening for the markupmutation event");
  let nodeMutated = inspector.once("markupmutation");
  let attr = container.editor.attrs[test.name].querySelector(".editable");
  setEditableFieldValue(attr, test.value, inspector);
  yield nodeMutated;

  info("Asserting the new attributes after edition");
  assertAttributes(test.node, test.expectedAttributes);

  info("Undo the change and assert that the attributes have been changed back");
  yield undoChange(inspector);
  assertAttributes(test.node, test.originalAttributes);

  info("Redo the change and assert that the attributes have been changed again");
  yield redoChange(inspector);
  assertAttributes(test.node, test.expectedAttributes);
}














function runEditOuterHTMLTests(tests, inspector) {
  info("Running " + tests.length + " edit-outer-html tests");
  return Task.spawn(function* () {
    for (let step of TEST_DATA) {
      yield runEditOuterHTMLTest(step, inspector);
    }
  });
}














function* runEditOuterHTMLTest(test, inspector) {
  info("Running an edit outerHTML test on '" + test.selector + "'");
  yield selectNode(test.selector, inspector);
  let oldNodeFront = inspector.selection.nodeFront;

  info("Listening for the markupmutation event");
  
  let mutated = inspector.once("markupmutation");
  info("Editing the outerHTML");
  inspector.markup.updateNodeOuterHTML(inspector.selection.nodeFront, test.newHTML, test.oldHTML);
  let mutations = yield mutated;
  ok(true, "The markupmutation event has fired, mutation done");

  info("Check to make the sure the correct mutation event was fired, and that the parent is selected");
  let nodeFront = inspector.selection.nodeFront;
  let mutation = mutations[0];
  let isFromOuterHTML = mutation.removed.some(n => n === oldNodeFront);

  ok(isFromOuterHTML, "The node is in the 'removed' list of the mutation");
  is(mutation.type, "childList", "Mutation is a childList after updating outerHTML");
  is(mutation.target, nodeFront, "Parent node is selected immediately after setting outerHTML");

  
  yield inspector.selection.once("new-node");

  
  
  let selectedNode = inspector.selection.node;
  let nodeFront = inspector.selection.nodeFront;
  let pageNode = getNode(test.selector);

  if (test.validate) {
    test.validate(pageNode, selectedNode);
  } else {
    is(pageNode, selectedNode, "Original node (grabbed by selector) is selected");
    is(pageNode.outerHTML, test.newHTML, "Outer HTML has been updated");
  }

  
  
  yield inspector.once("inspector-updated");
}
