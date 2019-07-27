



"use strict";





const TEST_PAGE = "data:text/html;charset=utf-8," +
                  "<p id=\"one\">one</p><p id=\"two\">two</p>";

add_task(function*() {
  let {inspector} = yield openInspectorForURL(TEST_PAGE);

  info("Making sure the markup-view frame is focused");
  inspector.markup._frame.focus();

  
  
  
  let highlightedNode = null;
  inspector.toolbox._highlighter.showBoxModel = function(nodeFront) {
    highlightedNode = nodeFront;
    return promise.resolve();
  };
  inspector.toolbox._highlighter.hideBoxModel = function() {
    return promise.resolve();
  };

  function* isHighlighting(selector, desc) {
    let nodeFront = yield getNodeFront(selector, inspector);
    is(highlightedNode, nodeFront, desc);
  }

  info("Hover over <p#one> line in the markup-view");
  yield hoverContainer("#one", inspector);
  yield isHighlighting("#one", "<p#one> is highlighted");

  info("Navigate to <p#two> with the keyboard");
  let onUpdated = inspector.once("inspector-updated");
  EventUtils.synthesizeKey("VK_DOWN", {}, inspector.panelWin);
  yield onUpdated;
  onUpdated = inspector.once("inspector-updated");
  EventUtils.synthesizeKey("VK_DOWN", {}, inspector.panelWin);
  yield onUpdated;
  yield isHighlighting("#two", "<p#two> is highlighted");

  info("Navigate back to <p#one> with the keyboard");
  onUpdated = inspector.once("inspector-updated");
  EventUtils.synthesizeKey("VK_UP", {}, inspector.panelWin);
  yield onUpdated;
  yield isHighlighting("#one", "<p#one> is highlighted again");
});
