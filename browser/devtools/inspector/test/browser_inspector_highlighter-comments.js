




"use strict";






thisTestLeaksUncaughtRejectionsAndShouldBeFixed("false");




const TEST_PAGE = TEST_URL_ROOT +
  "doc_inspector_highlighter-comments.html";

add_task(function* () {
  let {toolbox, inspector} = yield openInspectorForURL(TEST_PAGE);
  let markupView = inspector.markup;
  yield selectNode("p", inspector);

  info("Hovering over #id1 and waiting for highlighter to appear.");
  yield hoverElement("#id1");
  yield assertHighlighterShownOn("#id1");

  info("Hovering over comment node and ensuring highlighter doesn't appear.");
  yield hoverComment();
  yield assertHighlighterHidden();

  info("Hovering over #id1 again and waiting for highlighter to appear.");
  yield hoverElement("#id1");
  yield assertHighlighterShownOn("#id1");

  info("Hovering over #id2 and waiting for highlighter to appear.");
  yield hoverElement("#id2");
  yield assertHighlighterShownOn("#id2");

  info("Hovering over <script> and ensuring highlighter doesn't appear.");
  yield hoverElement("script");
  yield assertHighlighterHidden();

  info("Hovering over #id3 and waiting for highlighter to appear.");
  yield hoverElement("#id3");
  yield assertHighlighterShownOn("#id3");

  info("Hovering over hidden #id4 and ensuring highlighter doesn't appear.");
  yield hoverElement("#id4");
  yield assertHighlighterHidden();

  function hoverContainer(container) {
    let promise = inspector.toolbox.once("node-highlight");
    EventUtils.synthesizeMouse(container.tagLine, 2, 2, {type: "mousemove"},
        markupView.doc.defaultView);

    return promise;
  }

  function* hoverElement(selector) {
    info("Hovering node " + selector + " in the markup view");
    let container = yield getContainerForSelector(selector, inspector);
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

  function* assertHighlighterShownOn(selector) {
    let node = getNode(selector);
    let highlightNode = yield getHighlitNode(toolbox);
    is(node, highlightNode, "Highlighter is shown on the right node: " + selector);
  }

  function* assertHighlighterHidden() {
    let isVisible = yield isHighlighting(toolbox);
    ok(!isVisible, "Highlighter is hidden");
  }
});
