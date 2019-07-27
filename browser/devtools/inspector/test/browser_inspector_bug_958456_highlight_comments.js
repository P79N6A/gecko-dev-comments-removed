




"use strict";




const TEST_PAGE = TEST_URL_ROOT +
  "browser_inspector_bug_958456_highlight_comments.html";

let test = asyncTest(function* () {
  let { inspector } = yield openInspectorForURL(TEST_PAGE);
  let markupView = inspector.markup;
  yield selectNode("p", inspector);

  info("Hovering over #id1 and waiting for highlighter to appear.");
  yield hoverElement("#id1");
  assertHighlighterShownOn("#id1");

  info("Hovering over comment node and ensuring highlighter doesn't appear.");
  yield hoverComment();
  assertHighlighterHidden();

  info("Hovering over #id1 again and waiting for highlighter to appear.");
  yield hoverElement("#id1");
  assertHighlighterShownOn("#id1");

  info("Hovering over #id2 and waiting for highlighter to appear.");
  yield hoverElement("#id2");
  assertHighlighterShownOn("#id2");

  info("Hovering over <script> and ensuring highlighter doesn't appear.");
  yield hoverElement("script");
  assertHighlighterHidden();

  info("Hovering over #id3 and waiting for highlighter to appear.");
  yield hoverElement("#id3");
  assertHighlighterShownOn("#id3");

  info("Hovering over hidden #id4 and ensuring highlighter doesn't appear.");
  yield hoverElement("#id4");
  assertHighlighterHidden();

  function hoverContainer(container) {
    let promise = inspector.toolbox.once("node-highlight");
    EventUtils.synthesizeMouse(container.tagLine, 2, 2, {type: "mousemove"},
        markupView.doc.defaultView);

    return promise;
  }

  function hoverElement(selector) {
    info("Hovering node " + selector + " in the markup view");
    let container = getContainerForRawNode(markupView, getNode(selector));
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
    let node = getNode(selector);
    let highlightNode = getHighlitNode();
    is(node, highlightNode, "Highlighter is shown on the right node: " + selector);
  }

  function assertHighlighterHidden() {
    ok(!isHighlighting(), "Highlighter is hidden");
  }
});
