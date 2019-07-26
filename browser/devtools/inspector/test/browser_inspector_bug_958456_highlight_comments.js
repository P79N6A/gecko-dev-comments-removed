








let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
let {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
let {Task} = Cu.import("resource://gre/modules/Task.jsm", {});

const TEST_PAGE = "http://mochi.test:8888/browser/browser/devtools/inspector/test/browser_inspector_bug_958456_highlight_comments.html";
let inspector, markupView, doc;

function test() {
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    doc = content.document;

    waitForFocus(function() {
      openInspector((aInspector, aToolbox) => {
        inspector = aInspector;
        markupView = inspector.markup;
        startTests();
      });
    }, content);
  }, true);
  content.location = TEST_PAGE;
}

function startTests() {
  Task.spawn(function() {
    yield prepareHighlighter();

    
    yield hoverElement("#id1");
    assertHighlighterShownOn("#id1");

    
    yield hoverComment();
    assertHighlighterHidden();

    
    yield hoverElement("#id1");
    assertHighlighterShownOn("#id1");

    
    yield hoverElement("#id2");
    assertHighlighterShownOn("#id2");

    
    yield hoverElement("script");
    assertHighlighterHidden();

    
    yield hoverElement("#id3");
    assertHighlighterShownOn("#id3");

    
    yield hoverElement("#id4");
    assertHighlighterHidden();
  }).then(null, Cu.reportError).then(finishTest);
}

function finishTest() {
  doc = inspector = markupView = null;
  gBrowser.removeCurrentTab();
  finish();
}

function prepareHighlighter() {
  
  let deferred = promise.defer();
  inspector.selection.setNode(doc.querySelector("p"), null);
  inspector.once("inspector-updated", () => {
    deferred.resolve();
  });
  return deferred.promise;
}

function hoverContainer(container) {
  let deferred = promise.defer();
  EventUtils.synthesizeMouse(container.tagLine, 2, 2, {type: "mousemove"},
      markupView.doc.defaultView);
  inspector.toolbox.once("node-highlight", deferred.resolve);
  return deferred.promise;
}

function hoverElement(selector) {
  info("Hovering node " + selector + " in the markup view");
  let container = getContainerForRawNode(markupView, doc.querySelector(selector));
  return hoverContainer(container);
}

function hoverComment() {
  info("Hovering the comment node in the markup view");
  for (let [node, container] of markupView._containers) {
    if (node.nodeType === Ci.nsIDOMNode.COMMENT_NODE) {
      return hoverContainer(container);
    }
  }
}

function assertHighlighterShownOn(selector) {
  let node = doc.querySelector(selector);
  let highlightNode = getHighlitNode();
  is(node, highlightNode, "Highlighter is shown on the right node: " + selector);
}

function assertHighlighterHidden() {
  ok(!isHighlighting(), "Highlighter is hidden");
}
